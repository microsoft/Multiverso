#include "parameter_loader.h"

#include "double_buffer.h"
#include "log.h"
#include "msg_pack.h"
#include "multiverso.h"
#include "zmq_util.h"


namespace multiverso{

    ParameterLoaderBase::ParameterLoaderBase()
    {
    }

    ParameterLoaderBase::~ParameterLoaderBase()
    {
    }

    void ParameterLoaderBase::PushDataBlock(DataBlockBase *data_block)
    {
        data_queue_.Push(data_block);
    }

    void ParameterLoaderBase::RequestTable(integer_t table)
    {
        requests_.insert({ table, -1, -1 });
    }

    void ParameterLoaderBase::RequestRow(integer_t table, integer_t row)
    {
        if (requests_.find({ table, -1, -1 }) == requests_.end())
        {
            requests_.insert({ table, row, -1 });
        }
    }

    void ParameterLoaderBase::RequestElement(integer_t table,
        integer_t row, integer_t col)
    {
        if (requests_.find({ table, -1, -1 }) == requests_.end() &&
            requests_.find({ table, row, -1 }) == requests_.end())
        {
            requests_.insert({ table, row, col });
        }
    }

    void ParameterLoaderBase::ParseAndRequest(DataBlockBase* data_block)
    {
        Log::Fatal("You need to override this method\n");
    }

    void ParameterLoaderBase::StartThread()
    {
        DataBlockBase* data_block;
       
        while (data_queue_.Pop(data_block))
        {
            if (!Multiverso::is_pipeline_)
            {
                Multiverso::pipeline_barrier_->Wait();
            }
            if (data_block->Type() != DataBlockType::BeginClock
                && data_block->Type() != DataBlockType::EndClock) // skip Clock
            {
                BeginIteration();
                requests_.clear();
                // Parse data block
                ParseAndRequest(data_block);
                ProcessRequest();
                EndIteration();
            }
            if (!Multiverso::is_pipeline_)
            {
                Multiverso::pipeline_barrier_->Wait();
            }
        }
        if (!Multiverso::is_pipeline_)
        {
            Multiverso::pipeline_barrier_->Wait();
        }
    }

    void ParameterLoaderBase::Start()
    {
        loader_thread_ = std::thread(&ParameterLoaderBase::StartThread, this);
    }

    void ParameterLoaderBase::Stop()
    {
        data_queue_.Exit();
        loader_thread_.join();
    }

    // Underlying implemention of parameter request 
    void ParameterLoaderBase::ProcessRequest()
    {
        zmq::socket_t *socket = ZMQUtil::CreateSocket();
        std::vector<Table*>& cache =
            Multiverso::double_buffer_->IOBuffer();
        for (int i = 0; i < cache.size(); ++i)
        {
            cache[i]->Clear();
        }
        int src_rank = Multiverso::ProcessRank();
        int num_server = Multiverso::TotalServerCount();
        std::vector<MsgPack*> send_list(num_server, nullptr);
        std::vector<int> send_ret_size(num_server, 0);
        int num_send_msg = 0;
        for (auto tuple : requests_)
        {
            integer_t table = tuple.table;
            integer_t row = tuple.row;
            integer_t col = tuple.col;

            if (row >= 0 && requests_.find({ table, -1, -1 }) != requests_.end() ||
                col >= 0 && requests_.find({ table, row, -1 }) != requests_.end())
            {
                continue;
            }
            int dst_rank, last_rank;
            if (row == -1)
            {
                dst_rank = 0;
                last_rank = num_server - 1;
            }
            else
            {
                dst_rank = (table + row) % num_server;
                last_rank = dst_rank;
            }
            while (dst_rank <= last_rank)
            {
                if (send_list[dst_rank] == nullptr)
                {
                    send_list[dst_rank] = new MsgPack(MsgType::Get,
                        MsgArrow::Worker2Server, src_rank, dst_rank);
                    send_ret_size[dst_rank] = 0;
                }
                if (send_ret_size[dst_rank] + 3 * sizeof(integer_t) > kMaxMsgSize)
                {
                    send_list[dst_rank]->Send(socket);
                    ++num_send_msg;
                    delete send_list[dst_rank];
                    send_list[dst_rank] = new MsgPack(MsgType::Get,
                        MsgArrow::Worker2Server, src_rank, dst_rank);
                    send_ret_size[dst_rank] = 0;
                }
                zmq::message_t* msg = new zmq::message_t(3 * sizeof(integer_t));
                integer_t* buffer = static_cast<integer_t*>(msg->data());
                buffer[0] = table;
                buffer[1] = row;
                buffer[2] = col;
                send_list[dst_rank]->Push(msg);
                send_ret_size[dst_rank] += 3 * sizeof(integer_t);
                ++dst_rank;
            }
        }
        for (int i = 0; i < num_server; ++i)
        {
            if (send_ret_size[i] > 0)
            {
                send_list[i]->Send(socket);
                ++num_send_msg;
                delete send_list[i];
            }
        }

        // we expect each ReplyGet msg contains a over tag.
        while (num_send_msg > 0)
        {
            MsgPack reply(socket);
            for (int i = 1; i < reply.Size() - 1; ++i)
            {
                zmq::message_t* msg = reply.GetMsg(i);
                integer_t *buffer = static_cast<integer_t*>(msg->data());
                integer_t table = buffer[0];
                integer_t row = buffer[1];
                cache[table]->GetRow(row)->BatchAdd(buffer + 2);
            }
            zmq::message_t* msg = reply.GetMsg(reply.Size() - 1);
            bool over = (static_cast<integer_t*>(msg->data())[0] == 1);
            if (over)
            {
                --num_send_msg;
            }
        }
        delete socket;
    }

    void ParameterLoaderBase::BeginIteration()
    {
        Multiverso::double_buffer_->Start(0);
    } 

    void ParameterLoaderBase::EndIteration()
    {
        Multiverso::double_buffer_->End(0);
    }
}
