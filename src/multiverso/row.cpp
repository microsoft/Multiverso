#include "row.h"
#include "row_iter.h"
 
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cmath>
#include <unordered_map>

namespace multiverso
{
    //-- Row stuff definition ------------------------------------------------/
    template <typename T>
    Row<T>::Row(integer_t row_id, Format format, integer_t capacity, void* memory)
        : row_id_(row_id), format_(format), capacity_(capacity), memory_(memory), 
          nonzero_size_(0), num_deleted_key_(0), has_memory_(false)
    {
        if (format_ == Format::Dense)
        {
            if (memory_ == nullptr)
            {
                has_memory_ = true;
                memory_ = malloc(capacity * sizeof(T));
            }
            value_ = reinterpret_cast<T*>(memory_);
            memset(memory_, 0, capacity_ * sizeof(T));
        }
        else // format_ == Format::Sparse
        {
            if (memory_ == nullptr)
            {
                has_memory_ = true;
                memory_ = malloc(capacity * (sizeof(integer_t)+sizeof(T)));
            }
            key_ = reinterpret_cast<integer_t*>(memory_);
            memset(key_, 0, capacity_ * sizeof(integer_t));
            value_ = reinterpret_cast<T*>(key_ + capacity_);
        }
    }

    template <typename T>
    Row<T>::~Row()
    {
        if (has_memory_) 
        {
            free(memory_);
        }
    }

    template <typename T>
    int Row<T>::Add(integer_t key, void* delta)
    {
        return Add(key, *(reinterpret_cast<T*>(delta)));
    }

    template <typename T>
    int Row<T>::Add(integer_t key, T delta)
    {
        if (format_ == Format::Dense)
        {
            if (key < 0 || key >= capacity_)
            {
                return -1;
            }
            if (fabs(static_cast<double>(value_[key])) < kEPS)
            {
                ++nonzero_size_;
            }
            value_[key] += delta;
            if (fabs(static_cast<double>(value_[key])) < kEPS)
            {
                --nonzero_size_;
            }
        }
        else // format_ == Format::SPARSE
        {
            if (key < 0)
            {
                return -1;
            }
            integer_t internal_key = key + 1;
            integer_t bucket;
            if (FindPosition(internal_key, bucket))
            {
                value_[bucket] += delta;
                if (fabs(static_cast<double>(value_[bucket])) < kEPS) 
                {
                    // need to delete
                    key_[bucket] = kDeletedKey;
                    --nonzero_size_;
                    ++num_deleted_key_; // consider rehash when deleted much
                    if (num_deleted_key_ * 10 > capacity_)
                    {
                        Rehash();
                    }
                }
            }
            else // not found, then insert
            {
                key_[bucket] = internal_key;
                value_[bucket] = delta;
                ++nonzero_size_;
                if (nonzero_size_ * 2 > capacity_)
                {
                    Rehash(true); // double size of hash table
                }
            }
        }
        return 0;
    }

    template <typename T>
    int Row<T>::Add(void *delta)
    {
        if (format_ == Format::Dense)
        {
            T *buffer = static_cast<T*>(delta);
            nonzero_size_ = 0;
            for (int key = 0; key < capacity_; ++key)
            {
                value_[key] += buffer[key];
                if (fabs(static_cast<double>(value_[key])) > kEPS)
                {
                    ++nonzero_size_;
                }
            }
            return 0;
        }
        else
        {
            return -1;
        }
    }

    template <typename T>
    void Row<T>::At(integer_t key, void* value)
    {
        *(reinterpret_cast<T*>(value)) = At(key);
    }

    template <typename T>
    T Row<T>::At(integer_t key)
    {
        if (format_ == Format::Dense)
        {
            // assert(key >= 0 && key < capacity)
            return value_[key];
        }
        else // format_ == Format::SPARSE
        {
            integer_t internal_key = key + 1, bucket;
            if (FindPosition(internal_key, bucket))
            {
                return value_[bucket];
            }
            else
            {
                return 0;
            }
        }
    }

    template <typename T>
    integer_t Row<T>::Capacity()
    {
        return capacity_;
    }

    template <typename T>
    integer_t Row<T>::NonzeroSize()
    {
        return nonzero_size_;
    }

    template <typename T>
    void Row<T>::Clear()
    {
        num_deleted_key_ = 0;
        nonzero_size_ = 0;
        memset(memory_, 0, format_ == Format::Dense ?
            capacity_ * sizeof(T) : capacity_ * (sizeof(integer_t)+sizeof(T)));
    }

    template <typename T>
    integer_t Row<T>::RowId()
    {
        return row_id_;
    }

    template <typename T>
    Format Row<T>::RowFormat()
    {
        return format_;
    }

    template <typename T>
    typename RowBase::iterator* Row<T>::BaseIterator()
    {
        RowBase::iterator* iter = new RowIterator<T>(*this);
        return iter;
    }

    template <typename T>
    RowIterator<T> Row<T>::Iterator()
    {
        RowIterator<T> iter(*this);
        return iter;
    }

    template <typename T>
    void Row<T>::Serialize(void* byte)
    {
        // Format: n, col1, col2, ... col_n, val1, val2, ..., valn
        char* data = static_cast<char*>(byte);
        memcpy(data, &nonzero_size_, sizeof(integer_t));
        integer_t* col = 
            reinterpret_cast<integer_t*>(
            data + sizeof(integer_t));
        T* val = reinterpret_cast<T*>(
            data + sizeof(integer_t)* (nonzero_size_ + 1));
        iterator iter = Iterator();
        for (integer_t i = 0; iter.HasNext(); ++i)
        {
            col[i] = iter.Key();
            val[i] = iter.Value();
            iter.Next();
        }
    }

    template <typename T>
    void Row<T>::BatchAdd(void* byte)
    {
        integer_t size;
        memcpy(&size, byte, sizeof(integer_t));
        char* data = static_cast<char*>(byte);
        integer_t* col = reinterpret_cast<integer_t*>(data + sizeof(integer_t));
        T* val = reinterpret_cast<T*>(data + sizeof(integer_t)* (size + 1));
        for (integer_t i = 0; i < size; ++i)
        {
            Add(col[i], val[i]);
        }
    }

    template <typename T>
    std::string Row<T>::ToString()
    {
        std::string result = "";
        if (nonzero_size_ == 0)
        {
            return result;
        }
        result += std::to_string(row_id_);
        iterator iter = Iterator();
        for (integer_t i = 0; iter.HasNext(); ++i)
        {
            result += " " + std::to_string(iter.Key()) +
                ":" + std::to_string(iter.Value());
            iter.Next();
        }
        return result;
    }

    template <typename T>
    bool Row<T>::FindPosition(const integer_t key, int& bucket)
    {
        bool found = false; bucket = -1;    // REMARK: found never used, to be reviewed and fixed
        integer_t probes = 0;
        integer_t index = key % capacity_; // hash function
        while (true)
        {
            if (key_[index] == key)
            {
                bucket = index;
                return true; // found request key, return position of key
            }
            else if(key_[index] == kEmptyKey)
            {
                if (bucket == -1)
                {
                    bucket = index;
                }
                return false; // not found, return bucket is position to insert
            }
            else if (key_[index] == kDeletedKey)
            {
                if (bucket == -1)
                {
                    bucket = index; // keep searching, but mark record this
                }
            }
            ++probes;
            index = (index + 1) % capacity_; // liear probe
            assert(probes <= capacity_);
        }
    }

    template <typename T>
    void Row<T>::Rehash(bool resize)
    {
        if (format_ == Format::Dense)
        {
            return;
        }
        if (resize) // resize to double memory
        {
            void* new_memory = malloc(
                capacity_ * 2 * (sizeof(integer_t)+sizeof(T)));
            Row<T> new_row(row_id_, Format::Sparse, capacity_ * 2, new_memory);
            for (integer_t i = 0; i < capacity_; ++i)
            {
                if (key_[i] > 0)
                {
                    new_row.Add(key_[i] - 1, value_[i]);
                }
            }
            if (has_memory_)
            {
                free(memory_);
            }
            memory_ = new_memory;
            capacity_ <<= 1;
            has_memory_ = true;
            key_ = reinterpret_cast<integer_t*>(memory_);
            value_ = reinterpret_cast<T*>(key_ + capacity_);
            num_deleted_key_ = 0;
        }
        else // rehash in place
        {
            integer_t i = 0;
            while (i < capacity_)
            {
                if (key_[i] > 0) // key need to do rehash
                {
                    // do rehash 
                    integer_t index = key_[i] % capacity_;
                    while (key_[index] < -1)
                    {
                        index = (index + 1) % capacity_;
                    }
                    key_[i] = (-key_[i] - 1);
                    std::swap(key_[index], key_[i]);
                    std::swap(value_[index], value_[i]);
                }
                else
                {
                    ++i;
                }
            }
            for (integer_t i = 0; i < capacity_; ++i)
            {
                assert(key_[i] <= 0);
                if (key_[i] < -1)
                {
                    key_[i] = (-key_[i] - 1); // reverse key
                }
                if (key_[i] == kDeletedKey)
                {
                    key_[i] = kEmptyKey;
                }
            }
            num_deleted_key_ = 0;
        }
    }

    // explicity instantialize template class Row
    template class Row<int>;
    template class Row<int64_t>;
    template class Row<float>;
    template class Row<double>;

    //-- Row factory definition ----------------------------------------------/
    RowFactoryBase *RowFactoryBase::CreateRowFactory(Type type)
    {
        switch (type)
        {
        case Type::Int: return new RowFactory<int>();
        case Type::LongLong: return new RowFactory<int64_t>();
        case Type::Float: return new RowFactory<float>();
        case Type::Double: return new RowFactory<double>();
        default: return nullptr;
        }
    }

    template <typename T>
    RowBase *RowFactory<T>::CreateRow(integer_t row_id, Format format,
        integer_t capacity, void *memory)
    {
        return new Row<T>(row_id, format, capacity, memory);
    }
}
