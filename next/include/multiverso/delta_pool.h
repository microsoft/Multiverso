#ifndef MULTIVERSO_DELTA_POOL_H_
#define MULTIVERSO_DELTA_POOL_H_

/*!
 * \file delta_pool.h
 * \brief Defines DeltaPool data structure
 */

#include "meta.h"
#include "mt_queue.h"

#include <memory>

namespace multiverso
{
    /*!
     * \brief DeltaPool
     */
    class DeltaPool
    {
    public:
        /*! \brief Constructs a DeltaPool */
        explicit DeltaPool(int num_producer, int capacity = kDeltaPoolCapacity);
        ~DeltaPool();
        /*!
         * \brief Push a delta quadruple by an adaptor
         * \param trainer trainer id
         * \param table table id
         * \param row row id
         * \param col col id
         * \param delta pointer to the pushed value
         */
        void Push(int trainer, integer_t table,
            integer_t row, integer_t col, void* delta, int size);
        /*!
         * \brief Pop a delta quadruple by an aggregator
         * \param table returned table id
         * \param row returned row id
         * \param col returned col id
         * \param delta returned pointer to the pop value
         * \return true if Pop success, false if deltapool exit
         */
        bool Pop(integer_t& table,
            integer_t& row, integer_t& col, void*& delta);
        /*! \brief Exit the deltapool, awake all threads wait if necessary */
        void Exit();
    private:
        struct Entity;
        class EntityArray;
        typedef std::unique_ptr<EntityArray> Pointer;
        bool NeedFlush(int row);

        /*! \brief queue to cache available entity arrays */
        MtQueueMove<Pointer> empty_queue_;
        /*! \brief queue to store full entity arrays */
        MtQueueMove<Pointer> full_queue_;
        /*! \brief Each procucer has a pointer */
        std::vector<Pointer> producer_set_;
        /*! \brief consumer pointer*/
        Pointer consumer_;

        // No copying allowed
        DeltaPool(const DeltaPool&);
        void operator=(const DeltaPool&);
    };
}

#endif // MULTIVERSO_DELTA_POOL_H_
