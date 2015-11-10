#ifndef MULTIVERSO_META_H_
#define MULTIVERSO_META_H_

/*!
 * \file meta.h
 * \brief Includes some basic options and configurations.
 * \author feiyan, feiga
 */

#include <string>

namespace multiverso
{
/*!
 * \brief If defined the _MPI_VERSION_ macro, the communication rountine will 
 *        be compiled with MPI facilities, or ZMQ facilities will be used for 
 *        inter-process communication.
 */
#define _MPI_VERSION_

#define _MULTIVERSO_DEBUG_

    /*! 
     * \brief Defines the index type, could be 32-bit integer (int)
     * or 64-bit integer (e.g. long long).
     */
    typedef int integer_t;

    /*! 
     * \brief A very small number for comparision. Two values are regarded as
     * identical if their difference is within this gap.
     */
    const double kEPS = 1e-9;

    /*!
     * \brief Maximum data block size. An instance of MsgPack should be at 
     *        most around this size.
     */
    const int kMaxMsgSize = 1 << 25;    // 32 M

    /*! \brief Reserved size for Delta Array */
    const int kDeltaArraySize = 512 * 1024;  // 64 K

    /*! \brief Default size for Delta Pool */
    const int kDeltaPoolCapacity = 1024;

    /*!
     * \brief Row (data storage) format.
     * - DENSE: data will be stored with array;
     * - SPARSE: data will be stored with hash table.
     */
    enum class Format : int
    {
        Dense = 0,
        Sparse = 1 
    };

    /*! \brief Element type. */
    enum class Type : int
    {
        Int = 0,        ///< int: 32-bit integer
        LongLong = 1,   ///< long long: 64-bit integer
        Float = 2,      ///< float: 32-bit float
        Double = 3      ///< double: 64-bit float
    };

    /*! 
     * \brief DeltaType defines special type of Delta, which is the 
     *  communication entity between workers and aggregators.
     */
    enum class DeltaType : int
    {
        Flush = -1,     ///< flush: flush the cached updates to server
        Clock = -2      ///< clock: clock with other nodes
    };

    /*! \brief Defines message types. */
    enum class MsgType : int
    {
        // register message when initializing
        Register = 1,
        ReplyRegister = -1,
        // close message at the end
        Close = 2,
        ReplyClose = -2,
        // server table configuration messages
        CreateTable = 3,
        SetRow = 4,
        // add delta to the servers
        Add = 5,
        ReplyAdd = -5,  // QUESTION: is it necessary to have ReplyAdd option?
        // clock message for synchronization
        Clock = 6,
        ReplyClock = -6,
        // global inter-process barrier
        Barrier = 7,
        ReplyBarrier = -7,
        // parameter request messages
        Get = 8, 
        ReplyGet = -8,
        EndTrain = 9
    };

    /*!
    * \brief A enumeration class of lock options.
    */
    enum class LockOption : int
    {
        Immutable = 0,  // the threads do not write and there is no contention
        LockFree = 1,   // there is no lock for thread contention
        Locked = 2      // normal lock for thread contention
    };

    /*
    * \brief A simple data structure for passing the configuration parapmeters
    *        to Multiverso.
    */
    struct Config
    {
    public:
        Config()
        {
            num_servers = 0;    // all process are servers
            num_aggregator = 1;
            num_trainers = 1;
            max_delay = 0;
            num_lock = 100;
            lock_option = LockOption::Immutable;
            is_pipeline = true;
            server_endpoint_file = "";
        }

        /*
        * \brief Number of servers, only applied under MPI communication mode.
        *        If it is not greater than 0 or greater than the number of
        *        processes, all processes will have a server.
        */
        int num_servers;
        int num_aggregator;     ///< Number of aggregation threads.
        int num_trainers;       ///< Number of local trainer threads.
        int max_delay;          ///< the delay bound (max staleness)
        int num_lock;           ///< Number of locks in Locked option
        LockOption lock_option; ///< Lock option
        /// Whether overlapping the communication and computation
        bool is_pipeline;
        /// server ZMQ socket endpoint file in MPI-free version
        std::string server_endpoint_file;
    };

    struct RegisterInfo
    {
        int proc_rank;              // current process rank
        int proc_count;             // number of worker processes
        int server_count;           // number of servers
        int total_trainer_count;    // global number of trainers
    };

}

#endif // MULTIVERSO_META_H_
