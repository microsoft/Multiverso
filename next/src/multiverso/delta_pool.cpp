#include "delta_pool.h"

#include <cstring>

namespace multiverso
{
    /*!
    * \brief A delta entity, represented as a four tuple
    */
    struct DeltaPool::Entity
    {
        integer_t table;
        integer_t row;
        integer_t col;
        int delta[2];
    };

    /*!
    * \brief Entity array is a FIFO queue implemented by a fix-sized array
    *        This is used as a unit to push and pool in DeltaPool, to
    *        reduce the multithreads sync frequency.
    */
    class DeltaPool::EntityArray
    {
    public:
        EntityArray();
        ~EntityArray();
        /*!
        * \brief Push an entity in to queue
        * \return true if sucessfully, false if queue is full already
        */
        bool Push(integer_t table,
            integer_t row, integer_t col, void* value, int size);
        /*!
        * \brief Pop an entity from queue
        * \return true if sucessfully, false if queue is empty
        */
        bool Pop(integer_t& table,
            integer_t& row, integer_t& col, void*& value);
        /*!
        * \brief Clear the array
        */
        void Clear();
    private:
        /*! head is the position to pop */
        int head_;
        /*! tail is the position to push */
        int tail_;
        /*! underlying array */
        Entity* array_;
    };

    // -- EntityArray implementation area ---------------------------------- //
    DeltaPool::EntityArray::EntityArray()
        : head_(0), tail_(0)
    {
        array_ = new Entity[kDeltaArraySize];
    }

    DeltaPool::EntityArray::~EntityArray()
    {
        delete[] array_;
    }

    bool DeltaPool::EntityArray::Push(integer_t table,
        integer_t row, integer_t col, void* value, int size)
    {
        if (tail_ >= kDeltaArraySize)
        {
            return false;
        }
        array_[tail_].table = table;
        array_[tail_].row = row;
        array_[tail_].col = col;
        if (value != nullptr)
        {
            memcpy(array_[tail_].delta, value, size);
        }
        ++tail_;
        return true;
    }

    bool DeltaPool::EntityArray::Pop(integer_t& table,
        integer_t& row, integer_t& col, void*& value)
    {
        if (head_ >= tail_)
        {
            return false;
        }
        table = array_[head_].table;
        row = array_[head_].row;
        col = array_[head_].col;
        value = reinterpret_cast<void*>(&(array_[head_].delta));
        ++head_;
        return true;
    }

    void DeltaPool::EntityArray::Clear()
    {
        head_ = 0;
        tail_ = 0;
    }
    // -- End of EntityArray implementation area --------------------------- //

    // -- DeltaPool implementation area ------------------------------------ //
    DeltaPool::DeltaPool(int num_producer, int capacity)
        : producer_set_(num_producer)
    {
        for (int i = 0; i < capacity; ++i)
        {
            Pointer delta_array(new EntityArray);
            empty_queue_.Push(delta_array);
        }
    }

    DeltaPool::~DeltaPool() {}

    void DeltaPool::Push(int trainer, integer_t table,
        integer_t row, integer_t col, void* delta, int size)
    {
        Pointer& delta_array = producer_set_[trainer];
        if (!delta_array.get())
        {
            if (!empty_queue_.Pop(delta_array))
            {
                return;
            }
        }
        while (!delta_array->Push(table, row, col, delta, size))
        {
            full_queue_.Push(delta_array);
            if (!empty_queue_.Pop(delta_array))
            {
                return;
            }
        }
        if (NeedFlush(row))
        {
            full_queue_.Push(delta_array);
        }
    }

    bool DeltaPool::Pop(integer_t& table,
        integer_t& row, integer_t& col, void*& delta)
    {
        if (!consumer_.get())
        {
            if (!full_queue_.Pop(consumer_))
            {
                return false;
            }
        }
        while (!consumer_->Pop(table, row, col, delta))
        {
            consumer_->Clear();
            empty_queue_.Push(consumer_);
            if (!full_queue_.Pop(consumer_))
            {
                return false;
            }
        }
        return true;
    }

    void DeltaPool::Exit()
    {
        full_queue_.Exit();
        empty_queue_.Exit();
    }

    bool DeltaPool::NeedFlush(int row)
    {
        return static_cast<DeltaType>(row) == DeltaType::Flush ||
            static_cast<DeltaType>(row) == DeltaType::Clock;
    }
    // -- End of DeltaPool implementation area ----------------------------- //
}