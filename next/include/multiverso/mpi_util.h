#ifndef MULTIVERSO_MPI_UTIL_H_
#define MULTIVERSO_MPI_UTIL_H_

/*!
 * \file mpi_util.h
 * \brief Wraps the MPI facilities.
 * \author feiyan
 */

#include <queue>
#include <memory>
#if defined (_MPI_VERSION_)
#include <mpi.h>
#endif

namespace multiverso
{
    // MPI receive and send buffer size, for safe, one message should be at
    // most around half of this size, i.e. 32 MB
    const int kMPIBufferSize = 1 << 26; // 64 MB

    class MsgPack;

    /*! 
     * \brief The class MPIUtil provides the static method of using MPI 
     *        facilities.
     */
    class MPIUtil
    {
    public:
        /*!
         * \brief Initializes MPI environment.
         * \param argc Number of commandline arguments
         * \param argv Commandline arguments
         */
        static void Init(int *argc, char **argv[]);
        /*! \brief Finalizes MPI environment. */
        static void Close();

        /*! \brief Returns the MPI rank of current process. */
        static int MPIRank() { return mpi_rank_; }
        /*! \brief Returns the number of processes. */
        static int MPISize() { return mpi_size_; }

        static std::shared_ptr<MsgPack>ProbeAndRecv();

        /*! \brief Tests if there message received and returns it, or NULL. */
        //static std::shared_ptr<MsgPack> MPIProbe();
        /*! \brief Sends the message with MPI. Actually the message (if not 
         *         NULL) will be pushed into the send queue and MPI will try to
         *         send one in the queue when call the method once. So if you
         *         call Send(nullptr), the MPI will try to send one message in
         *         the queue (like flush but only try to send one)
         */
        static void Send(std::shared_ptr<MsgPack> msg_pack);
        /*! \brief Returns the size of send queue. */
        static int SendQueueSize() { return static_cast<int>(send_queue_.size()); }

    private:
        static int mpi_size_;
        static int mpi_rank_;

        static std::queue<std::shared_ptr<MsgPack>> send_queue_;

#if defined (_MPI_VERSION_)
        static char recv_buffer_[kMPIBufferSize];
        static MPI_Request recv_request_;
        static char send_buffer_[kMPIBufferSize];
        static MPI_Request send_request_;
#endif
    };
}

#endif // MULTIVERSO_MPI_UTIL_H_
