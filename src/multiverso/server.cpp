#include <algorithm>
#include "log.h"
#include "row.h"
#include "table.h"
#include "table_iter.h"
#include "zmq_util.h"
#include "msg_pack.h"
#include "server.h"
#include "stop_watch.h"

namespace multiverso
{
    // Initializes the basic info and starts a server thread.
    Server::Server(int server_id, int num_worker_process, std::string endpoint) : 
        clocks_(num_worker_process, 0), 
        clock_msg_(num_worker_process, nullptr),
        lock_pool_(41)
    {
        server_id_ = server_id;
        worker_proc_count_ = num_worker_process;
        endpoint_ = endpoint;
        max_delay_ = -1;

        is_working_ = false;
        inited_ = false;
        server_thread_ = std::thread(&Server::StartThread, this);
        // keep waiting until the server thread goes into the working routine
        while (!is_working_) 
        {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }

    // Closes and destroy the server
    Server::~Server()
    {
        is_working_ = false;
        WaitToComplete();
        Log::Info("Server %d closed.\n", server_id_);
    }

    // Waits for the server thread completed
    void Server::WaitToComplete()
    {
        if (server_thread_.joinable())
        {
            server_thread_.join();
        }
    }

    // Initialization at the beginning of the server thread.
    void Server::Init()
    {
        // communication stuffs
        router_ = new zmq::socket_t(ZMQUtil::GetZMQContext(), ZMQ_ROUTER);
        router_->bind(endpoint_.c_str());
        poll_count_ = 1;
        poll_items_ = new zmq::pollitem_t[poll_count_];
        // for vs2013
        poll_items_[0] = { static_cast<void*>(*router_), 0, ZMQ_POLLIN, 0 };
        // for vs2012
        //poll_items_[0].socket = *router_;
        //poll_items_[0].fd = 0;
        //poll_items_[0].events = ZMQ_POLLIN;
        //poll_items_[0].revents = 0;
        update_thread_ = std::thread(&Server::StartUpdateThread, this);
        Log::Info("Server %d starts: num_workers=%d endpoint=%s\n", 
            server_id_, worker_proc_count_, endpoint_.c_str());
    }

    // Clean up at the end of the server thread
    void Server::Clear()
    {
        update_queue_.Exit();
        if (update_thread_.joinable())
        {
            update_thread_.join();
        }
        delete router_;
        delete []poll_items_;
        // Dump model before Clear
        DumpModel();
        for (auto &table : tables_)
        {
            delete table;
        }
    }

    // Server background thread function
    void Server::StartThread()
    {
        MsgType msg_type;
        MsgArrow arrow;
        int src, dst;

        Init();
        std::vector<zmq::socket_t*> sockets;
        sockets.push_back(router_);
        std::queue<std::shared_ptr<MsgPack>> msg_queue;
        is_working_ = true;
        while (is_working_)
        {
            ZMQUtil::ZMQPoll(poll_items_, poll_count_, sockets, msg_queue);
            while (!msg_queue.empty())
            {
                std::shared_ptr<MsgPack> msg_pack = msg_queue.front();
                msg_queue.pop();
                msg_pack->GetHeaderInfo(&msg_type, &arrow, &src, &dst);
                switch (msg_type)
                {
                case MsgType::Register: Process_Register(msg_pack); break;
                case MsgType::Close: Process_Close(msg_pack); break;
                case MsgType::Barrier: inited_ = Process_Barrier(msg_pack); break;
                case MsgType::CreateTable: Process_CreateTable(msg_pack); break;
                case MsgType::SetRow: Process_SetRow(msg_pack); break;
                case MsgType::Clock: Process_Clock(msg_pack); break;
                case MsgType::EndTrain: Process_EndTrain(msg_pack); break;
                case MsgType::Get: Process_Get(msg_pack); break;
                case MsgType::Add: inited_ ? update_queue_.Push(msg_pack) 
                    : Process_Add(msg_pack); break;
                // case MsgType::Add: Process_Add(msg_pack); break;
                // other message process...
                }
            }
        }
        Clear();
    }

    void Server::StartUpdateThread()
    {
        std::shared_ptr<MsgPack> msg_pack;
        while (update_queue_.Pop(msg_pack))
        {
            Process_Add(msg_pack);
        }
    }

    // REMARK (feiyan): consider a Register pattern
    //-- Server process routine ---------------------------------------------//
    // processes register message
    void Server::Process_Register(std::shared_ptr<MsgPack> msg_pack)
    {
        waiting_msg_.push_back(msg_pack);
        // If all expected workers come, process the information, assign them
        // IDs and reply all back
        if (waiting_msg_.size() == worker_proc_count_)
        {
            // basic statistics
            int total_trainers = 0;
            int server_count = 0;
            for (auto &msg : waiting_msg_)
            {
                int *buffer = static_cast<int*>(msg->GetMsg(1)->data());
                total_trainers += buffer[0];    // sum up number of trainers
                if (server_count == 0)          // verify the number of servers
                {
                    server_count = buffer[1];
                }
                else if (server_count != buffer[1])
                {
                    server_count = -1;
                }
                max_delay_ = std::max<int>(max_delay_, buffer[2]);
            }
            // reply each worker process
            for (int rank = 0; rank < waiting_msg_.size(); ++rank)
            {
                MsgPack *reply = waiting_msg_[rank]->CreateReplyMsgPack();
                zmq::message_t *msg = new zmq::message_t(4 * sizeof(int));
                int *buffer = static_cast<int*>(msg->data());
                buffer[0] = rank;               // assigned process rank
                buffer[1] = worker_proc_count_; // number of worker processes
                buffer[2] = server_count;       // number of servers
                buffer[3] = total_trainers;     // total number of trainers
                reply->Push(msg);
                reply->Send(router_);
                delete reply;
            }
            waiting_msg_.clear();

            Log::Info(
                "Server %d: Worker registratrion completed: workers=%d trainers=%d servers=%d\n",
                server_id_, worker_proc_count_, total_trainers, server_count);
        } // end if
    }

    // processes close message
    void Server::Process_Close(std::shared_ptr<MsgPack> msg_pack)
    {
        MsgType msg_type;
        MsgArrow arrow;
        int src, dst;
        msg_pack->GetHeaderInfo(&msg_type, &arrow, &src, &dst);
        Log::Info("Server %d: Received close message from worker %d.\n", server_id_, src);
        // if the last worker comes, stop the server thread
        if (Process_Barrier(msg_pack))
        {
            is_working_ = false;
        }
    }

    // process barrier message, returns true if the last one comes, 
    // or false otherwise
    bool Server::Process_Barrier(std::shared_ptr<MsgPack> msg_pack)
    {
        waiting_msg_.push_back(msg_pack);
        // if the last expected worker comes, reply all pending workers
        if (waiting_msg_.size() == worker_proc_count_)
        {
            for (auto &msg : waiting_msg_)
            {
                MsgPack *reply = msg->CreateReplyMsgPack();
                reply->Send(router_);
                delete reply;
            }
            waiting_msg_.clear();
            return true;
        }
        return false;
    }

    // process create table message
    void Server::Process_CreateTable(std::shared_ptr<MsgPack> msg_pack)
    {
        integer_t table_id, rows, cols;
        Type type;
        Format format;
        integer_t *buf = static_cast<integer_t*>(msg_pack->GetMsg(1)->data());
        table_id = buf[0];                          // table_id
        rows = buf[1];                              // rows
        cols = buf[2];                              // cols
        int *buf_i = static_cast<int*>(buf + 3);
        type = static_cast<Type>(buf_i[0]);         // type
        format = static_cast<Format>(buf_i[1]);     // default format

        // the calling should be in order starting from 0, process the first
        // come and ignore the others
        if (table_id == tables_.size())
        {
            tables_.push_back(new Table(table_id, rows, cols, type, format));
        }
    }

    // process set row message
    void Server::Process_SetRow(std::shared_ptr<MsgPack> msg_pack)
    {
        integer_t table_id, row_id, capacity;
        Format format;
        for (int idx = 1; idx < msg_pack->Size(); ++idx)
        {
            integer_t *buf = static_cast<integer_t*>(msg_pack->GetMsg(idx)->data());
            table_id = buf[0];                      // table_id
            row_id = buf[1];                        // row_id
            capacity = buf[2];                      // capacity
            memcpy(&format, buf + 3, sizeof(int));  // format
            tables_[table_id]->SetRow(row_id, format, capacity);
        }
    }

    void Server::Process_Clock(std::shared_ptr<MsgPack> msg_pack)
    {
        MsgType msg_type;
        MsgArrow arrow;
        int src, dst;
        msg_pack->GetHeaderInfo(&msg_type, &arrow, &src, &dst);

        if (max_delay_ < 0) // ASP model, reply immediately
        {
            MsgPack *reply = msg_pack->CreateReplyMsgPack();
            reply->Send(router_);
            delete reply;
        }
        else
        {
            clock_msg_[src] = msg_pack;
            clocks_[src]++;
            int upper_bound =
                *std::min_element(clocks_.begin(), clocks_.end()) + max_delay_;
            for (int worker = 0; worker < worker_proc_count_; ++worker)
            {
                if (clocks_[worker] <= upper_bound && clock_msg_[worker] != nullptr)
                {
                    MsgPack *reply = clock_msg_[worker]->CreateReplyMsgPack();
                    reply->Send(router_);
                    delete reply;
                    clock_msg_[worker] = nullptr;
                }
            }
        }
    }

    void Server::Process_EndTrain(std::shared_ptr<MsgPack> msg_pack)
    {
        MsgType msg_type;
        MsgArrow arrow;
        int src, dst;
        msg_pack->GetHeaderInfo(&msg_type, &arrow, &src, &dst);
        clocks_[src] = 1 << 30;
        int upper_bound =
            *std::min_element(clocks_.begin(), clocks_.end()) + max_delay_;
        for (int worker = 0; worker < worker_proc_count_; ++worker)
        {
            if (clocks_[worker] <= upper_bound && clock_msg_[worker] != nullptr)
            {
                MsgPack *reply = clock_msg_[worker]->CreateReplyMsgPack();
                reply->Send(router_);
                delete reply;
                clock_msg_[worker] = nullptr;
            }
        }
    }
    
    // processes get message
    void Server::Process_Get(std::shared_ptr<MsgPack> msg_pack)
    {
        MsgPack* reply = msg_pack->CreateReplyMsgPack();
        int send_ret_size = 0;
        for (int i = 1; i < msg_pack->Size(); ++i)
        {
            zmq::message_t* msg = msg_pack->GetMsg(i);
            integer_t* buffer = static_cast<integer_t*>(msg->data());
            integer_t table = buffer[0];
            integer_t row = buffer[1];
            integer_t col = buffer[2];
            if (row == -1)
            {
                TableIterator iter = tables_[table]->Iterator();
                while (iter.HasNext())
                {
                    integer_t request_row = iter.RowId();
                    lock_pool_.Lock(row);
                    Process_GetRow(table, request_row, col, msg_pack, 
                        send_ret_size, reply);
                    lock_pool_.Unlock(row);
                    iter.Next();
                }
            }
            else
            {
                lock_pool_.Lock(row);
                Process_GetRow(table, row, col, msg_pack, send_ret_size, reply);
                lock_pool_.Unlock(row);
            }
        }
        zmq::message_t *tail = new zmq::message_t(sizeof(integer_t));
        integer_t* over_tag = static_cast<integer_t*>(tail->data());
        *over_tag = 1;
        reply->Push(tail);
        reply->Send(router_);
        delete reply;
    }

    // processing get row 
    void Server::Process_GetRow(integer_t table, integer_t row, integer_t col,
        std::shared_ptr<MsgPack> msg_pack, int& send_ret_size, MsgPack*& reply)
    {
        integer_t row_size = (col >= 0) ? 1 :
            tables_[table]->GetRow(row)->NonzeroSize();
        int msg_size = sizeof(integer_t)* 3 + row_size *
            (sizeof(integer_t)+tables_[table]->ElementSize());
        if (send_ret_size + msg_size > kMaxMsgSize)
        {
            zmq::message_t *tail = new zmq::message_t(sizeof(integer_t));
            integer_t* over_tag = static_cast<integer_t*>(tail->data());
            *over_tag = 0;
            reply->Push(tail);
            reply->Send(router_);
            delete reply;
            reply = msg_pack->CreateReplyMsgPack();
            send_ret_size = 0;
        }
        zmq::message_t *row_msg = new zmq::message_t(msg_size);
        integer_t *buffer = static_cast<integer_t*>(row_msg->data());
        // Format: table_id, row_id, number
        //         col_1, col2, ..., col_n, val_1, val_2, ..., val_n;
        buffer[0] = table;
        buffer[1] = row;
        if (col >= 0)
        {
            buffer[2] = 1;
            buffer[3] = col;
            tables_[table]->GetRow(row)->At(col, buffer + 4);
        }
        else
        {
            tables_[table]->GetRow(row)->Serialize(buffer + 2);
        }
        reply->Push(row_msg);
        send_ret_size += msg_size;
    }

    // processes add message
    void Server::Process_Add(std::shared_ptr<MsgPack> msg_pack)
    {
        for (int i = 1; i < msg_pack->Size(); ++i)
        {
            zmq::message_t* msg = msg_pack->GetMsg(i);
            integer_t* buffer = static_cast<integer_t*>(msg->data());
            integer_t table = buffer[0];
            integer_t row = buffer[1];
            lock_pool_.Lock(row);
            tables_[table]->GetRow(row)->BatchAdd(buffer + 2);
            lock_pool_.Unlock(row);
        }
    }
    //-- end of server process routine ---------------------------------------/
    
    void Server::DumpModel()
    {
        Log::Info("Server %d: Dump model...\n", server_id_);
        for (int i = 0; i < tables_.size(); ++i)
        {
            // dump model to file server_$server_id$_table_$table_id$.model
            std::string file_name = "server_" + std::to_string(server_id_)
                + "_table_" + std::to_string(i) + ".model";
            std::ofstream fout(file_name);

            Table *table = tables_[i];
            TableIterator iter = table->Iterator();
            int size = table->ElementSize();
            while (iter.HasNext())
            {
                fout << iter.Row()->ToString() << std::endl;
                iter.Next();
            }
            fout.close();
        }
    }
}
