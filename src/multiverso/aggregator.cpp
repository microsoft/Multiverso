#include "aggregator.h"
#include "barrier.h"
#include "delta_pool.h"
#include "meta.h"
#include "msg_pack.h"
#include "multiverso.h"
#include "table.h"
#include "table_iter.h"
#include "row.h"
#include "row_iter.h"
#include "vector_clock.h"
#include "zmq_util.h"

#include "zmq.hpp"

#include <unordered_map>

namespace multiverso
{
    Aggregator::Aggregator(int num_threads, int num_trainers)
        : done_(false), num_threads_(num_threads),
        num_trainers_(num_trainers), thread_counter_(0)
    {
        barrier_ = new Barrier(num_threads_);
        for (int i = 0; i < num_threads; ++i)
        {
            delta_pools_.push_back(new DeltaPool(
                num_trainers_, kDeltaPoolCapacity));
            threads_.push_back(std::thread(&Aggregator::StartThread, this));
        }
    }

    Aggregator::~Aggregator()
    {
        done_ = true;
        for (int i = 0; i < num_threads_; ++i)
        {
            delta_pools_[i]->Exit();
            threads_[i].join();
            delete delta_pools_[i];
        }
        for (auto& table : tables_)
        {
            delete table;
        }
        delete barrier_;
    }

    void Aggregator::CreateTable(integer_t table_id, integer_t rows,
        integer_t cols, Type type, Format default_format,
        int64_t memory_pool_size)
    {
        if (table_id >= tables_.size())
        {
            tables_.resize(table_id + 1);
        }
        tables_[table_id] = new Table(table_id,
            rows, cols, type, default_format, memory_pool_size);
    }

    int Aggregator::SetAggregatorRow(integer_t table_id, integer_t row_id,
        Format format, integer_t capacity)
    {
        return tables_[table_id]->SetRow(row_id, format, capacity);
    }

    void Aggregator::Add(int trainer,
        integer_t table, integer_t row, integer_t col, void* delta)
    {
        if (row >= 0)
        {
            int i = row % num_threads_;
            delta_pools_[i]->Push(trainer,
                table, row, col, delta, tables_[table]->ElementSize());
        }
        else
        {
            for (auto& delta_pool : delta_pools_)
            {
                // used col to record the trainer id
                delta_pool->Push(trainer, table, row, trainer, nullptr, 0);
            }
        }
    }

    void Aggregator::StartThread()
    {
        int id = thread_counter_++;
        VectorClock flush_vector(num_trainers_);
        VectorClock clock_vector(num_trainers_);
        DeltaPool* delta_pool = delta_pools_[id];

        barrier_->Wait();
        
        integer_t table, row, col;
        void* delta;
        zmq::socket_t* socket = ZMQUtil::CreateSocket();

        while (delta_pool->Pop(table, row, col, delta))
        {
            switch (static_cast<DeltaType>(row))
            {
            case DeltaType::Flush:
                if (flush_vector.Update(col))
                {
                    Send(id, socket);
                    if (barrier_->Wait())
                    {
                        for (auto& table : tables_)
                        {
                            table->Clear();
                        }
                    }
                    barrier_->Wait();
                }
                break;
            case DeltaType::Clock:
                if (clock_vector.Update(col))
                {
                    if (barrier_->Wait())
                    {
                        Clock(socket);
                    }
                    barrier_->Wait();
                }
                break;
            default: // the general delta type, add update to aggregator table
                assert(row >= 0);
                tables_[table]->GetRow(row)->Add(col, delta);
            }
        }
        delete socket;
    }

    void Aggregator::Send(int id, zmq::socket_t* socket)
    {
        int src_rank = Multiverso::ProcessRank();
        int num_server = Multiverso::TotalServerCount();

        std::vector<MsgPack*> send_list(num_server, nullptr);
        std::vector<int> send_ret_size(num_server, 0);

        for (int table_id = 0; table_id < tables_.size(); ++table_id)
        {
            Table* table = tables_[table_id];
            TableIterator iter(*table);

            for (; iter.HasNext(); iter.Next())
            {
                integer_t row_id = iter.RowId();
                if (row_id % num_threads_ != id)
                {
                    continue;
                }
                RowBase* row = iter.Row();

                int dst_rank = (table_id + row_id) % num_server;
                if (send_list[dst_rank] == nullptr)
                {
                    send_list[dst_rank] = new MsgPack(MsgType::Add,
                        MsgArrow::Worker2Server, src_rank, dst_rank);
                    send_ret_size[dst_rank] = 0;
                }
                // Format: table_id, row_id, number
                //         col_1, col2, ..., col_n, val_1, val_2, ..., val_n;
                int msg_size = sizeof(integer_t)* 3 + row->NonzeroSize() *
                    (table->ElementSize() + sizeof(integer_t));
                if (msg_size > kMaxMsgSize) 
                {
                    // TODO(feiga): we currently assume the row serialized size
                    // not ecceed kMaxMsgSize. should solve the issue later.
                    Log::Error("Row size exceed the max size of message\n");
                }
                if (send_ret_size[dst_rank] + msg_size > kMaxMsgSize)
                {
                    send_list[dst_rank]->Send(socket);
                    delete send_list[dst_rank];
                    send_list[dst_rank] = new MsgPack(MsgType::Add,
                        MsgArrow::Worker2Server, src_rank, dst_rank);
                    send_ret_size[dst_rank] = 0;
                }
                zmq::message_t* msg = new zmq::message_t(msg_size);
                integer_t* buffer = static_cast<integer_t*>(msg->data());
                buffer[0] = table_id;
                buffer[1] = row_id;
                row->Serialize(buffer + 2);
                send_list[dst_rank]->Push(msg);
                send_ret_size[dst_rank] += msg_size;
            }
        }
        for (int i = 0; i < num_server; ++i)
        {
            if (send_ret_size[i] > 0)
            {
                send_list[i]->Send(socket);
                delete send_list[i];
            }
        }
    }

    void Aggregator::Clock(zmq::socket_t* socket)
    {
        MsgPack* msg = new MsgPack(MsgType::Clock, MsgArrow::Worker2Server,
            Multiverso::ProcessRank(), 0);
        msg->Send(socket);     // Send the clock message
        delete msg;
        MsgPack reply(socket); // Wait for reply
        // signal conditions variable to awake trainer process
        std::unique_lock<std::mutex> lock(mutex_);
        sync_cv_.notify_all();

    }

    void Aggregator::Wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        sync_cv_.wait(lock);
    }
}