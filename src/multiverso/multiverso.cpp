#include <algorithm>
#include "zmq.hpp"
#include "log.h"
#include "lock.h"
#include "msg_pack.h"
#include "communicator.h"
#include "aggregator.h"
#include "server.h"
#include "double_buffer.h"
#include "zmq_util.h"
#include "multiverso.h"

namespace multiverso
{
    //-- Static member definition area ---------------------------------------/
    RegisterInfo Multiverso::reg_info_;
    int Multiverso::num_trainers_;

    zmq::socket_t *Multiverso::socket_ = nullptr;
    LockManager *Multiverso::lock_manager_ = nullptr;
    LockOption Multiverso::lock_option_ = LockOption::Immutable;
    Communicator *Multiverso::communicator_ = nullptr;
    Aggregator *Multiverso::aggregator_ = nullptr;

    bool Multiverso::is_pipeline_ = true;
    Barrier* Multiverso::pipeline_barrier_ = nullptr;
    std::vector<TrainerBase*> Multiverso::trainers_;
    ParameterLoaderBase *Multiverso::param_loader_ = nullptr;

    std::vector<Table*> Multiverso::tables0_;
    std::vector<Table*> Multiverso::tables1_;
    DoubleBuffer<std::vector<Table*>> *Multiverso::double_buffer_ = nullptr;

    std::mutex Multiverso::mutex_;
    std::condition_variable Multiverso::wait_cv_;
    std::vector<bool> Multiverso::data_tag_;

    std::vector<MsgPack*> Multiverso::row_config_;
    std::vector<int> Multiverso::row_config_size_;
    int Multiverso::row_config_count_ = 0;
    //-- End of static member definition area --------------------------------/


    // Initializes Multiverso environment
    int Multiverso::Init(std::vector<TrainerBase*> &trainers,
        ParameterLoaderBase *param_loader, const Config &config, 
        int *argc, char **argv[])
    {
#if defined (_MULTIVERSO_DEBUG_)
        Log::ResetLogLevel(LogLevel::Debug);
#endif

        int ret = 0;
        num_trainers_ = static_cast<int>(trainers.size());
        // Starts the background communicator
        communicator_ = new Communicator(config, argc, argv);
        socket_ = communicator_->CreateSocket();
        reg_info_ = communicator_->Register(socket_, config, num_trainers_);
        // Starts the background aggregator
        aggregator_ = new Aggregator(config.num_aggregator, num_trainers_);

        row_config_.resize(reg_info_.server_count, nullptr);
        row_config_size_.resize(reg_info_.server_count, 0);

        is_pipeline_ = config.is_pipeline;
        pipeline_barrier_ = is_pipeline_ ? nullptr : new Barrier(num_trainers_+1);

        // prepare local data structures
        data_tag_.resize(trainers.size());
        trainers_ = trainers;
        param_loader_ = param_loader;
        double_buffer_ = new DoubleBuffer<std::vector<Table*>>(
            static_cast<int>(trainers.size()), &tables0_, &tables1_);
        if ((lock_option_ = config.lock_option) == LockOption::Locked)
        {
            lock_manager_ = new LockManager(std::max<int>(config.num_lock, 1));
        }

        if (ret == 0)
        {
            Log::Info("Rank %d/%d: Multiverso initialized successfully.\n", 
                reg_info_.proc_rank, reg_info_.proc_count);
        }
        return ret;
    }

    int Multiverso::Init(const Config &config, int *argc, char **argv[])
    {
#if defined (_MULTIVERSO_DEBUG_)
        Log::ResetLogLevel(LogLevel::Debug);
#endif
        int ret = 0;
        num_trainers_ = static_cast<int>(config.num_trainers);
        // Starts the background communicator
        communicator_ = new Communicator(config, argc, argv);
        socket_ = communicator_->CreateSocket();
        reg_info_ = communicator_->Register(socket_, config, num_trainers_);
        // Starts the background aggregator
        aggregator_ = new Aggregator(config.num_aggregator, num_trainers_);

        row_config_.resize(reg_info_.server_count, nullptr);
        row_config_size_.resize(reg_info_.server_count, 0);

        // prepare local data structures
        double_buffer_ = new DoubleBuffer<std::vector<Table*>>(
            static_cast<int>(num_trainers_), &tables0_, &tables1_);
        if ((lock_option_ = config.lock_option) == LockOption::Locked)
        {
            lock_manager_ = new LockManager(std::max<int>(config.num_lock, 1));
        }

        if (ret == 0)
        {
            Log::Info("Rank %d/%d: Multiverso initialized successfully.\n", 
                reg_info_.proc_rank, reg_info_.proc_count);
        }
        return ret;
    }

    // Close Multiverso environment
    void Multiverso::Close(bool finalize) 
    {
        // Sends close message to all servers
        for (int server = 0; server < reg_info_.server_count; ++server)
        {
            MsgPack msg(MsgType::Close, MsgArrow::Worker2Server, 
                reg_info_.proc_rank, server);
            msg.Send(socket_);
        }
        for (int server = 0; server < reg_info_.server_count; ++server)
        {
            MsgPack reply(socket_);
        }
        delete socket_;

        // close the background threads
        delete aggregator_;
        communicator_->SetMPIFinalize(finalize);
        delete communicator_;

        // delete data structures
        delete double_buffer_;
        for (int i = 0; i < tables0_.size(); ++i)
        {
            delete tables0_[i];
            delete tables1_[i];
        }
        tables0_.clear();
        tables1_.clear();
        if (lock_manager_)
        {
            delete lock_manager_;
            lock_manager_ = nullptr;
        }

        Log::Info("Rank %d/%d: Multiverso closed successfully.\n", 
            reg_info_.proc_rank, reg_info_.proc_count);
    }

    void Multiverso::BeginConfig() 
    {
        Log::Info("Rank %d/%d: Begin of configuration and initialization.\n", 
            reg_info_.proc_rank, reg_info_.proc_count);
    }

    // Global barrier at the end of initialization
    void Multiverso::EndConfig() 
    {
        FlushSetServerRow();

        // Send flush and clock signal to aggregator, let the aggregator send
        // the update to server.
        for (int trainer = 0; trainer < num_trainers_; ++trainer)
        {
            aggregator_->Add(trainer, 0, -1, 0, nullptr);   // flush
            aggregator_->Add(trainer, 0, -2, 0, nullptr);   // clock
        }
        aggregator_->Wait();

        // Send barrier messages to all servers
        // DISCUSSION: it may be better to let the Aggregator to do the barrier.
        for (int server = 0; server < reg_info_.server_count; ++server)
        {
            MsgPack msg(MsgType::Barrier, MsgArrow::Worker2Server, 
                reg_info_.proc_rank, server);
            msg.Send(socket_);
        }
        // wait for replies
        for (int server = 0; server < reg_info_.server_count; ++server)
        {
            MsgPack reply(socket_);
        }
        Log::Info("Rank %d/%d: End of configration and initialization.\n", 
            reg_info_.proc_rank, reg_info_.proc_count);
    }

    void Multiverso::AddTable(integer_t table, integer_t rows, integer_t cols,
        Type type, Format default_format, int64_t memory_pool_size)
    {
        AddServerTable(table, rows, cols, type, default_format);
        AddCacheTable(
            table, rows, cols, type, default_format, memory_pool_size);
        AddAggregatorTable(
            table, rows, cols, type, default_format, memory_pool_size);
    }

    // Sends messages to each server for creating a server table.
    void Multiverso::AddServerTable(integer_t table, integer_t rows, 
        integer_t cols, Type type, Format default_format)
    {
        for (int server = 0; server < reg_info_.server_count; ++server)
        {
            MsgPack msg_pack(MsgType::CreateTable, MsgArrow::Worker2Server, 
                reg_info_.proc_rank, server);
            zmq::message_t *msg = 
                new zmq::message_t(3 * sizeof(integer_t) + 2 * sizeof(int));
            integer_t *buf = static_cast<integer_t*>(msg->data());
            buf[0] = table;                                 // table id
            buf[1] = rows;                                  // rows
            buf[2] = cols;                                  // cols
            int *buf_i = static_cast<int*>(buf + 3);
            buf_i[0] = static_cast<int>(type);              // type
            buf_i[1] = static_cast<int>(default_format);    // default_format
            msg_pack.Push(msg);
            msg_pack.Send(socket_);
        }
    }

    // Creates local cache tables for double buffer
    int Multiverso::AddCacheTable(integer_t table, integer_t rows, integer_t cols,
        Type type, Format default_format, int64_t memory_pool_size)
    {
        if (table == tables0_.size())
        {
            tables0_.push_back(new Table(
                table, rows, cols, type, default_format, memory_pool_size));
            tables1_.push_back(new Table(
                table, rows, cols, type, default_format, memory_pool_size));
            return 0;
        }
        return -1;
    }

    // Creates aggregator table
    void Multiverso::AddAggregatorTable(integer_t table, integer_t rows, 
        integer_t cols, Type type, Format default_format, 
        int64_t memory_pool_size)
    {
        aggregator_->CreateTable(
            table, rows, cols, type, default_format, memory_pool_size);
    }

    // The unified method of setting a row.
    void Multiverso::SetRow(integer_t table, integer_t row, Format format,
        integer_t capacity)
    {
        SetServerRow(table, row, format, capacity);
        SetCacheRow(table, row, format, capacity);
        SetAggregatorRow(table, row, format, capacity);
    }

    // Caches a server row setting message and sends them to the server if the
    // message pack is large enough.
    void Multiverso::SetServerRow(integer_t table, integer_t row, 
        Format format, integer_t capacity)
    {
        int server = (table + row) % reg_info_.server_count;
        if (row_config_[server] == nullptr)
        {
            row_config_[server] = new MsgPack(MsgType::SetRow, 
                MsgArrow::Worker2Server, reg_info_.proc_rank, server);
            ++row_config_count_;
        }

        int size = 3 * sizeof(integer_t)+sizeof(int);
        zmq::message_t *msg = new zmq::message_t(size);
        integer_t *buf = static_cast<integer_t*>(msg->data());
        buf[0] = table;                         // table_id
        buf[1] = row;                           // row_id
        buf[2] = capacity;                      // capacity
        memcpy(buf + 3, &format, sizeof(int));  // format
        row_config_[server]->Push(msg);
        row_config_size_[server] += size;

        if (row_config_size_[server] >= kMaxMsgSize)
        {
            row_config_[server]->Send(socket_);
            delete row_config_[server];
            row_config_[server] = nullptr;
            row_config_size_[server] = 0;
            --row_config_count_;
        }
    }

    void Multiverso::FlushSetServerRow()
    {
        for (int server = 0; server < reg_info_.server_count; ++server)
        {
            if (row_config_[server] != nullptr)
            {
                row_config_[server]->Send(socket_);
                delete row_config_[server];
                row_config_[server] = nullptr;
                row_config_size_[server] = 0;
                --row_config_count_;
            }
        }
    }

    // Configures a row in cache.
    int Multiverso::SetCacheRow(integer_t table, integer_t row, 
        Format format, integer_t capacity)
    {
        int ret0 = tables0_[table]->SetRow(row, format, capacity);
        int ret1 = tables1_[table]->SetRow(row, format, capacity);
        return (ret0 < 0 || ret1 < 0) ? -1 : 0;
    }

    // Configures a row in aggregator.
    int Multiverso::SetAggregatorRow(integer_t table, integer_t row, 
        Format format, integer_t capacity)
    {
        return aggregator_->SetAggregatorRow(table, row, format, capacity);
    }

    void Multiverso::AddToServerPtr(
        integer_t table, integer_t row, integer_t col, void *delta)
    {
        if (row_config_count_ > 0)
        {
            FlushSetServerRow();
        }
        aggregator_->Add(0, table, row, col, delta);
    }

    void Multiverso::Flush()
    {
        for (int trainer = 0; trainer < trainers_.size(); ++trainer)
        {
            aggregator_->Add(trainer, 0, -1, 0, nullptr);   // flush
        }
    }

    // Begin training
    void Multiverso::BeginTrain()
    {
        Log::Info("Rank %d/%d: Begin of training.\n", 
            reg_info_.proc_rank, reg_info_.proc_count);
        param_loader_->Start();
        for (auto &trainer : trainers_)
        {
            trainer->Start();
        }
    }

    // End training
    void Multiverso::EndTrain()
    {
        // REMARK(feiyan): should make sure that the param loader and trainers 
        //                 consumed all the data
        // REMARK(feiga):  the stop order is important
        param_loader_->Stop();
        for (auto &trainer : trainers_)
        {
            trainer->Stop();
        }

        MsgPack msg(MsgType::EndTrain, MsgArrow::Worker2Server, 
            reg_info_.proc_rank, 0);
        msg.Send(socket_);
        
        Log::Info("Rank %d/%d: End of training.\n", 
            reg_info_.proc_rank, reg_info_.proc_count);
    }

    void Multiverso::BeginClock()
    {
        static DataBlockBase begin_clock(DataBlockType::BeginClock);
        PushDataBlock(&begin_clock);
    }

    void Multiverso::EndClock()
    {
        static DataBlockBase end_clock(DataBlockType::EndClock);
        PushDataBlock(&end_clock);
    }

    void Multiverso::PushDataBlock(DataBlockBase *data_block)
    {
        // mark the data block as being processed
        data_block->IncreaseCount(1);

        param_loader_->PushDataBlock(data_block);
        for (auto &trainer : trainers_)
        {
            trainer->PushDataBlock(data_block);
        }
    }

    void Multiverso::Wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        wait_cv_.wait(lock, [] {
            for (auto tag : data_tag_) if (!tag) return false;
            return true;
        });
    }
}
