#include "msg_pack.h"
#include "mpi_util.h"

//#include <exception>
//#include "log.h" // temporary include


namespace multiverso
{
    //-- area of static member definition ------------------------------------/
    int MPIUtil::mpi_size_ = 0;
    int MPIUtil::mpi_rank_ = 0;
    std::queue<std::shared_ptr<MsgPack>> MPIUtil::send_queue_;
#if defined (_MPI_VERSION_)
    char MPIUtil::recv_buffer_[kMPIBufferSize];
    MPI_Request MPIUtil::recv_request_ = MPI_REQUEST_NULL;

    char MPIUtil::send_buffer_[kMPIBufferSize];
    MPI_Request MPIUtil::send_request_ = MPI_REQUEST_NULL;
#endif
    //-- end of static member definition -------------------------------------/

#if defined (_MPI_VERSION_)
    // Initializes MPI environment
    void MPIUtil::Init(int *argc, char **argv[])
    {
        int flag = 0;
        MPI_Initialized(&flag); // test if MPI has been initialized
        if (!flag) // if MPI not started, start it
        {
            MPI_Init_thread(argc, argv, MPI_THREAD_SERIALIZED, &flag);
        }
        MPI_Comm_size(MPI_COMM_WORLD, &mpi_size_);
        MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank_);
    }

    // Finalizes MPI environment
    void MPIUtil::Close()
    {
        MPI_Finalize();
    }

    std::shared_ptr<MsgPack> MPIUtil::ProbeAndRecv()
    {
        int flag;
		int count = 0;
        MPI_Status status;
        // test if there is new message comes
        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
        if (flag) // MPI message arrived
        {
            MPI_Recv(recv_buffer_, kMPIBufferSize, MPI_BYTE, status.MPI_SOURCE,
                status.MPI_TAG, MPI_COMM_WORLD, &status);
			MPI_Get_count(&status, MPI_BYTE, &count);	
            std::shared_ptr<MsgPack> request(
                new MsgPack(recv_buffer_, count));
            return request;
        }
        return nullptr;
    }

    //// Try to receive messages with MPI non-blocking receiving method.
    //std::shared_ptr<MsgPack> MPIUtil::MPIProbe()
    //{
    //    try{
    //        int flag;
	//        int count = 0;
    //        MPI_Status status;
    //        // if there is message being received
    //        if (recv_request_ != MPI_REQUEST_NULL)
    //        {
    //            // test if the receiving completed
    //            MPI_Test(&recv_request_, &flag, &status);
    //            if (flag) // recv completed, deserialize the data into ZMQ messages
    //            {
    //                recv_request_ = MPI_REQUEST_NULL;
			//MPI_Get_count(&status, MPI_BYTE, &count);	
    //                std::shared_ptr<MsgPack> request(
    //                    new MsgPack(recv_buffer_, count));
    //                return request;
    //            }
    //            else // recv not completed yet
    //            {
    //                return nullptr;
    //            }
    //        }

    //        // test if there is new message comes
    //        MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &flag, &status);
    //        if (flag) // MPI message arrived
    //        {
    //            //MPI_Irecv(recv_buffer_, kMPIBufferSize, MPI_BYTE, status.MPI_SOURCE,
    //            //    status.MPI_TAG, MPI_COMM_WORLD, &recv_request_);
    //            MPI_Recv(recv_buffer_, kMPIBufferSize, MPI_BYTE, status.MPI_SOURCE, 
    //                status.MPI_TAG, MPI_COMM_WORLD, &status);
	//            MPI_Get_count(&status, MPI_BYTE, &count);	
    //            std::shared_ptr<MsgPack> request(new MsgPack(recv_buffer_, count));
    //            return request;
    //        }
    //        return nullptr;
    //    }
    //    catch (std::exception e)
    //    {
    //        Log::Error("Rank=%d Probe\n", mpi_rank_);
    //        throw e;
    //    }
    //}

    // Send messages with MPI non-blocking sending method. Actually, it pushes 
    // the message into the queue (if not null), test if last sending has
    // been completed, and send a new one in the queue if so.
    void MPIUtil::Send(std::shared_ptr<MsgPack> msg_pack)
    {
        static int tag = 0;

        // push the send message into the send queue
        if (msg_pack.get() != nullptr)
        {
            send_queue_.push(msg_pack);
        }

        // test if the last send has been completed, return immediately if not
        if (send_request_ != MPI_REQUEST_NULL)
        {
            MPI_Status status;
            int flag;
            MPI_Test(&send_request_, &flag, &status);
            if (flag) // send completed
            {
                send_request_ = MPI_REQUEST_NULL;
            }
            else
            {
                return;
            }
        }

        // if there is message in the send queue, send it
        if (!send_queue_.empty())
        {
            MsgType msg_type;
            MsgArrow msg_arrow;
            int src, dst, size;
            std::shared_ptr<MsgPack> msg_send = send_queue_.front();
            send_queue_.pop();
            msg_send->GetHeaderInfo(&msg_type, &msg_arrow, &src, &dst);
            // serialize the message into MPI send buffer
            msg_send->Serialize(send_buffer_, &size);
            MPI_Isend(send_buffer_, size, MPI_BYTE, dst, tag++, MPI_COMM_WORLD,
                &send_request_);
        }        
    }
#endif
}
