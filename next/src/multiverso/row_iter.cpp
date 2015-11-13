#include "row_iter.h"
#include "row.h"

#include <cstdint>
#include <cmath>

namespace multiverso
{
    template <typename T>
    RowIterator<T>::RowIterator(Row<T>& row)
        : row_(row), index_(0)
    {
        SkipInvalidValue();
    }

    template <typename T>
    RowIterator<T>::~RowIterator() {}

    template <typename T>
    bool RowIterator<T>::HasNext()
    {
        return index_ < row_.Capacity();
    }

    template <typename T>
    void RowIterator<T>::Next()
    {
        ++index_;
        SkipInvalidValue();
    }

    template <typename T>
    integer_t RowIterator<T>::Key()
    {
        return row_.RowFormat() == Format::Dense ? 
            index_ : row_.key_[index_] - 1;
    }

    template <typename T>
    void RowIterator<T>::Value(void* value)
    {
        T return_value = Value();
        *(reinterpret_cast<T*>(value)) = return_value;
    }

    template <typename T>
    T RowIterator<T>::Value()
    {
        return row_.value_[index_];
    }

    template <typename T>
    void RowIterator<T>::SkipInvalidValue()
    {
        if (row_.RowFormat() == Format::Dense)
        {
            while (index_ < row_.Capacity() &&  // skip zero 
                fabs(static_cast<double>(row_.value_[index_])) < kEPS)
            {
                ++index_;
            }
        }
        else // row.RowFormat() == Format::SPARSE
        {
            while (index_ < row_.Capacity() &&
                row_.key_[index_] <= 0) // skip empty key or deleted key
            {
                ++index_;
            }
        }
    }

    // template class explicit instantiation
    template class RowIterator<int>;
    template class RowIterator<int64_t>;
    template class RowIterator<float>;
    template class RowIterator<double>;
}