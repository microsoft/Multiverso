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

    /*!
     * \brief Aggregator is responsble for collecting, aggregating, storing
     *        and sending local updates. 
     */
    class IAggregator
    {
    public:
        /*!
         * \brief Construct an aggregator.
         * \param num_threads number of background aggregator threads
         * \param num_trainers number of trainer thread
         */
        virtual ~IAggregator() = default;

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
         * \brief Add to 
         * \param trainer trainer id
         * \param table table id
         * \param row row id
         * \param col col id
         * \param delta pointer to delta
         */
        virtual void Add(int trainer, integer_t table,
            integer_t row, integer_t col, void* delta) = 0;

        virtual void BatchAdd(int trainer, integer_t table,
            integer_t row, void* row_delta) = 0;

        virtual void Flush(int trainer) = 0;

        virtual void Clock(int trainer) = 0;

        /*! \brief Wait until all client finish sync up */
        virtual void Wait() {}

        static IAggregator* CreateAggregator(
            int num_aggregators, int num_trainers);
    protected:
        std::vector<Table*> tables_;    // buffer to store update 

        /*! \brief Send local updates */
        void Send(int id, zmq::socket_t* socket, int num_threads = 1);
    };
}

#endif // MULTIVERSO_AGGREGATOR_H_
