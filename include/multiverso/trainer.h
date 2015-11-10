#ifndef MULTIVERSO_TRAINER_H_
#define MULTIVERSO_TRAINER_H_

/*!
 * \file trainer.h
 * \brief Defines the TrainerBase interface.
 * \author feiyan
 */

#include <atomic>
#include <thread>
#include <mutex>
#include <vector>
#include "meta.h"
#include "mt_queue.h"

namespace multiverso
{
    class DataBlockBase;
    class RowBase;
    template <typename T>
    class Row;
    class Barrier;
    class Table;

    /*! 
     * \brief The TrainerBase class defines an interface for user-customized
     *        Trainer classes.
     */
    class TrainerBase
    {
    public:
        /*! \brief Assigns a trainer id to the instance. */
        TrainerBase();
        virtual ~TrainerBase();

        /*!
         * \brief Users override this method to train/update the model given a
         * block of data, or other customized logic routine. This method is
         * called by Multiverso working threads at each iteration (for each
         * train/test data block).
         */
        virtual void TrainIteration(DataBlockBase* data_block) {}
        /*! 
         * \brief This method will be called at the beginning of each clock 
         * period (signaled by a trivial data block of type "BEGINCLOCK").
         */
        virtual void BeginClock() {}
        /*!
         * \brief This method will be called at the end of each clock period
         * (signaled by a trivial data block of type "ENDCLOCK").
         */
        virtual void EndClock() {}

        /*! \brief Returns the id of the trainer instance (indexed from 0).*/
        int TrainerId() const { return trainer_id_; }
        /*! \brief Returns the number of local trainers.*/
        int TrainerCount() const { return trainer_count_; }

        /*! \brief Returns a pointer to the specified table */
        Table *GetTable(integer_t table_id);

        /*! \brief Returns a reference to a specific row.*/
        template <typename T>
        Row<T> &GetRow(integer_t table_id, integer_t row_id)
        {
            Row<T> *row = static_cast<Row<T>*>(GetRowPtr(table_id, row_id));
            return *row;
        }

        /*! \brief Adds an element delta to a specific element.*/
        template <typename T>
        void Add(integer_t table_id, integer_t row_id, integer_t col_id, T delta)
        {
            AddPtr(table_id, row_id, col_id, &delta);
        }

        /*! \brief Adds a row delta to a specified row. */
        template <typename T>
        void Add(integer_t table_id, integer_t row_id, std::vector<T> &delta)
        {
            Add(table_id, row_id, delta.data());
        }

        /*! \brief Adds a row delta to a specified row. */
        void Add(integer_t table_id, integer_t row_id, void *delta);

        /*! \brief Finished a clock period of training and sync.*/
        void Clock();
        void BeginIteration();
        void EndIteration();

        /*! 
         * \brief Push a data block into the data queue of the trainer.
         * \param data_block A data block for training in an iteration (or 
         *        other logic such as test or clock signals).
         */
        void PushDataBlock(DataBlockBase *data_block);

        /*! \brief Starts a training thread.*/
        void Start();
        /*! \brief Stops the training thread.*/
        void Stop();

    private:
        void StartThread();

        /*! \brief Returns a pointer to the specified row. */
        RowBase *GetRowPtr(integer_t table_id, integer_t row_id);
        /*! \brief Adds a delta to the specific element with pointer manner. */
        void AddPtr(integer_t table_id, integer_t row_id, integer_t col_id, 
            void *delta);

        // -- area of member variables ---------------------------------------/
        std::thread trainer_thread_;
        MtQueueMove<DataBlockBase*> data_queue_;
        int trainer_id_;

        static std::atomic<int> trainer_count_;  // the total trainer count in local
        static std::mutex mtx_;
        static Barrier barrier_;

        // No copying allowed
        TrainerBase(const TrainerBase&);
        void operator=(const TrainerBase&);
    };
}

#endif // MULTIVERSO_TRAINER_H_
