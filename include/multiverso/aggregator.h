#ifndef MULTIVERSO_AGGREGATOR_H_
#define MULTIVERSO_AGGREGATOR_H_

/*!
* \brief Defines aggregator
*/

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "meta.h"

namespace zmq
{
    class socket_t;
}

namespace multiverso
{
    class Table;
    class DeltaPool;
    class Barrier;

    /*!
     * \brief Aggregator is responsble for collecting, aggregating, storing
     *        and sending local updates. It provides interface for trainers to
     *        concurrently add delta to local cache (almost) without confilict.
     *        and also provide pipeline for training and networking.
     */
    class Aggregator
    {
    public:
        /*!
         * \brief Construct an aggregator.
         * \param num_threads number of background aggregator threads
         * \param num_trainers number of trainer thread
         */
        Aggregator(int num_threads, int num_trainers);
        ~Aggregator();

        /*!
         * \brief Create table for local updates aggregation
         * \param table_id Table identity
         * \param rows Number of rows
         * \param cols Number of columns
         * \param type Element type
         * \param default_format Default row format
         * \param memory_pool_size Memory pool size. When creating rows,
         *        the table will reuse the memory pool if available.
         */
        void CreateTable(integer_t table_id, integer_t rows, integer_t cols, 
            Type type, Format default_format, int64_t memory_pool_size = 0);

        /*! 
         * \brief Configures a row of aggregation tables
         * \param table_id Table id
         * \param row_id Row id
         * \param format Row format.
         * \param capacity The initial maximal number of elements the row
         *        stores, it is fixed for DENSE row (array), and can be
         *        auto-growed for SPARSE row (hash table).
         * \return Returns 0 on success, or -1 with error.
         */
        int SetAggregatorRow(integer_t table_id, integer_t row_id,
            Format format, integer_t capacity);

        /*!
         * \brief Push sparse delta into queues. The delta would be depatched
         *        to different aggregator thread based on the row_id. The
         *        process is (in spirit) similar with MapReduce.
         * \param trainer trainer id
         * \param table table id
         * \param row row id
         * \param col col id
         * \param delta pointer to delta
         */
        void Add(int trainer,
            integer_t table, integer_t row, integer_t col, void* delta);

        /*! \brief Wait until all client finish sync up */
        void Wait();
    private:
        /*! \brief Entrance function of aggregator threads */
        void StartThread();
        /*! \brief Send local updates */
        void Send(int id, zmq::socket_t* socket);
        /*! \brief Clock to server */
        void Clock(zmq::socket_t* socket);
    private:
        bool done_;                     // whether aggregators is done 
        int num_threads_;               // number of aggregator threads
        int num_trainers_;              // number of trainers
        std::vector<Table*> tables_;    // buffer to store update 
        std::vector<DeltaPool*> delta_pools_;
        std::vector<std::thread> threads_;
        std::atomic<int> thread_counter_;
        /*! \brief synchronization barrier used by multiple aggregator */
        Barrier* barrier_;
        std::mutex mutex_;
        std::condition_variable sync_cv_;
        bool sync_;

        // No copying allowed
        Aggregator(const Aggregator&);
        void operator=(const Aggregator&);
    };
}

#endif // MULTIVERSO_AGGREGATOR_H_
