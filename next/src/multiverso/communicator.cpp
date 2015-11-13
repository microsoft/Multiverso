#include "meta.h"
#include "log.h"
#include "server.h"
#include "zmq_util.h"
#include "mpi_util.h"
#include "msg_pack.h"
#include "endpoint_list.h"
#include "communicator.h"

namespace multiverso
{
    // Creates Communicator object and starts communication thread.
    Communicator::Communicator(const Config &config, int *argc, char **argv[])
    {
        reg_info_.proc_rank = -1;
        reg_info_.proc_count = -1;
        reg_info_.server_count = -1;
        server_ = nullptr;

#if defined (_MPI_VERSION_)
        // starts MPI first
        MPIUtil::Init(argc, argv);
        reg_info_.proc_rank = MPIUtil::MPIRank();
        reg_info_.proc_count = MPIUtil::MPISize();
        mpi_finalize_ = true;

        // Starts an affiliated server
        reg_info_.server_count = config.num_servers;
        if (reg_info_.server_count >= 0
            || reg_info_.server_count >= reg_info_.proc_count)
        {
            reg_info_.server_count = reg_info_.proc_count;
        }
        if (reg_info_.proc_rank < reg_info_.server_count)
        {
            server_ = new Server(reg_info_.proc_rank, reg_info_.proc_count, 
                kSERVER_ENDPOINT.c_str());
        }
#endif

        // starts communication thread
        is_working_ = false;
        comm_thread_ = std::thread([=](){
            StartThread(config.server_endpoint_file);
        });
        // keeps waiting until the communicator thread goes into the working routine
        while (!is_working_)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // Destroies the Communicator object.
    Communicator::~Communicator()
    {
        delete server_;

        // POTENTIAL ISSUE: if stoping the communicator too early, there might
        //                  be unprocessed messages. We should design a method
        //                  to make sure that all messages have been sent.
        std::this_thread::sleep_for(std::chrono::seconds(1));
        is_working_ = false;
        comm_thread_.join();
#if defined (_MPI_VERSION_)
        if (mpi_finalize_)
        {
            MPIUtil::Close();
        }
#endif
    }

    RegisterInfo Communicator::Register(zmq::socket_t *socket, 
        const Config &config, int num_trainers)
    {
        if (reg_info_.server_count < 0)
        {
            reg_info_.server_count = static_cast<int>(dealers_.size());
        }

        // send a register message to server 0
        // message format: num_local_trainers:int, num_servers:int, max_delay:int
        MsgPack msg_pack(MsgType::Register, MsgArrow::Worker2Server, 
            reg_info_.proc_rank, 0);
        zmq::message_t *msg = new zmq::message_t(3 * sizeof(int));
        int *buffer = static_cast<int*>(msg->data());
        buffer[0] = static_cast<int>(num_trainers); // number of local trainers
        buffer[1] = reg_info_.server_count; // local server count configuration
        buffer[2] = config.max_delay;   // delay bound (max staleness)
        msg_pack.Push(msg);
        msg_pack.Send(socket);

        // get the reply
        MsgPack reply(socket);
        buffer = static_cast<int*>(reply.GetMsg(1)->data());
        if (reg_info_.proc_rank < 0)
        {
            reg_info_.proc_rank = buffer[0];
        }
        if (reg_info_.proc_count < 0)
        {
            reg_info_.proc_count = buffer[1];
        }
        if (reg_info_.server_count != buffer[2])
        {
            Log::Fatal(
                "Rank %d/%d: Inconsistance in number of servers: local=%d vs. global=%d\n",
                reg_info_.proc_rank, reg_info_.proc_count, 
                reg_info_.server_count, buffer[2]);
        }
        reg_info_.total_trainer_count = buffer[3];

        return reg_info_;
    }

    //int Communicator::MPIRank() { return MPIUtil::MPIRank(); }
    //int Communicator::MPISize() { return MPIUtil::MPISize(); }

    zmq::socket_t *Communicator::CreateSocket() 
    { 
        return ZMQUtil::CreateSocket(); 
    }

    // Initialization when starting the communication thread.
    void Communicator::Init(std::string server_endpoint_file)
    {
        router_ = new zmq::socket_t(ZMQUtil::GetZMQContext(), ZMQ_ROUTER);
        router_->bind(kCOMM_ENDPOINT.c_str());

#if defined (_MPI_VERSION_)
        dealers_.push_back(
            new zmq::socket_t(ZMQUtil::GetZMQContext(), ZMQ_DEALER));
        dealers_[0]->connect(kSERVER_ENDPOINT.c_str());
#else
        EndpointList endpoint_list(server_endpoint_file);
        for (int server_id = 0; server_id < endpoint_list.Size(); ++server_id)
        {
            dealers_.push_back(
                new zmq::socket_t(ZMQUtil::GetZMQContext(), ZMQ_DEALER));
            dealers_[server_id]->connect(
                ("tcp://" + endpoint_list.GetEndpoint(server_id)).c_str());
        }
#endif
        
        poll_count_ = static_cast<int>(dealers_.size() + 1);
        poll_items_ = new zmq::pollitem_t[poll_count_];
        poll_items_[0] = { static_cast<void*>(*router_), 0, ZMQ_POLLIN, 0 };
        // poll_items_[0].socket = static_cast<void*>(*router_);
        for (int server_id = 0; server_id < dealers_.size(); ++server_id)
        {
            poll_items_[server_id + 1] = { static_cast<void*>(*dealers_[server_id]), 0, ZMQ_POLLIN, 0 };
        }
    }

    // Cleaning up when closing the communication thread.
    void Communicator::Clear()
    {
        delete router_;
        for (auto &item : dealers_)
        {
            delete item;
        }
        delete []poll_items_;
    }

    // Starts the communication thread.
    void Communicator::StartThread(std::string server_endpoint_file)
    {
        Init(server_endpoint_file);
        // prepare the polling stuffs
        std::vector<zmq::socket_t*> sockets;
        sockets.push_back(router_);
        for (auto &item : dealers_)
        {
            sockets.push_back(item);
        }
        std::queue<std::shared_ptr<MsgPack>> msg_queue;
        std::shared_ptr<MsgPack> msg_pack = nullptr;
        MsgType type;
        MsgArrow arrow;
        int src, dst;

        // starts working
        is_working_ = true;
#if defined (_MPI_VERSION_)
        while (is_working_ || MPIUtil::SendQueueSize() > 0)
        {
            // Probe zmq. If there are too many messages in MPI send queue, 
            // postpone processing the coming messages
            if (MPIUtil::SendQueueSize() < 128)
            {
                ZMQUtil::ZMQPoll(poll_items_, poll_count_, sockets, msg_queue);
            }
            // probe mpi
            msg_pack = MPIUtil::ProbeAndRecv();
            if (msg_pack.get() != nullptr)
            {
                msg_queue.push(msg_pack);
            }
            // process the messages in the queue
            while (!msg_queue.empty())
            {
                msg_pack = msg_queue.front();
                msg_queue.pop();
                msg_pack->GetHeaderInfo(&type, &arrow, &src, &dst);
                // If the destination is not the current process, send the
                // message to external process with MPI. Or send the message to
                // internal threads according to the message arrow.
                if (dst != reg_info_.proc_rank)
                {
                    MPIUtil::Send(msg_pack);
                }
                else if (arrow == MsgArrow::Worker2Server)
                {
                    msg_pack->Send(dealers_[0]);
                }
                else // server -> worker
                {
                    msg_pack->Send(router_);
                }
            }
            // let MPI send the message in send queue
            MPIUtil::Send(nullptr);
        }
#else
        while (is_working_)
        {
            ZMQUtil::ZMQPoll(poll_items_, poll_count_, sockets, msg_queue);
            while (!msg_queue.empty())
            {
                msg_pack = msg_queue.front();
                msg_queue.pop();
                msg_pack->GetHeaderInfo(&type, &arrow, &src, &dst);
                if (arrow == MsgArrow::Worker2Server)
                {
                    msg_pack->Send(dealers_[dst]);
                }
                else // server -> worker
                {
                    msg_pack->Send(router_);
                }
            }
        }
#endif

        Clear();
    }
}
