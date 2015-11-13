#include <memory.h>
#include <cstdlib>
#include <algorithm>
#include "log.h"
#include "row.h"
#include "table_iter.h"
#include "table.h"

namespace multiverso
{    
    RowInfo::RowInfo(Format _format, integer_t _capacity, integer_t _index)
    {
        this->format = _format;
        this->capacity = _capacity;
        this->index = _index;
        this->row = nullptr;
    }

    Table::Table(integer_t table_id, integer_t rows, integer_t cols, Type type,
        Format default_format, int64_t memory_pool_size)
    {
        table_id_ = table_id;

        // create memory pool
        memory_size_ = std::max<int64_t>(memory_pool_size, 0);
        used_memory_size_ = 0;
        memory_pool_ = (memory_size_ > 0) ? (new char[memory_size_]) : nullptr;

        row_factory_ = RowFactoryBase::CreateRowFactory(type);
        element_size_ = row_factory_->ElementSize();

        // set default row configuration
        integer_t capacity = (default_format == Format::Dense) ? cols : 2;
        row_info_.reserve(rows);
        for (integer_t row = 0; row < rows; ++row)
        {
            row_info_.emplace_back(default_format, capacity);
        }
    }

    Table::~Table()
    {
        Clear();
        delete row_factory_;
        delete memory_pool_;
    }

    int Table::SetRow(integer_t row_id, Format format, integer_t capacity)
    {
        if (row_id < 0 || row_id >= row_info_.size())
        {
            return -1;
        }
        row_info_[row_id].format = format;
        row_info_[row_id].capacity = capacity;
        return 0;
    }

    RowBase* Table::GetRow(integer_t row_id)
    {
        if (row_id < 0 || row_id >= row_info_.size())
        {
            return nullptr;
        }
        // if row is not created
        if (row_info_[row_id].index < 0)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (row_info_[row_id].index < 0) // double check
            {
                int64_t row_size = (row_info_[row_id].format == Format::Dense) ?
                    (row_info_[row_id].capacity * element_size_) :
                    (row_info_[row_id].capacity * (sizeof(integer_t)+element_size_));
                char *pt = nullptr;
                if (used_memory_size_ + row_size <= memory_size_)
                {
                    pt = memory_pool_ + used_memory_size_;
                    used_memory_size_ += row_size;
                }
                RowBase *row = row_factory_->CreateRow(
                    row_id, row_info_[row_id].format, row_info_[row_id].capacity, pt);
                rows_.push_back(row);
                row_info_[row_id].row = row;
                row_info_[row_id].index = static_cast<integer_t>(rows_.size() - 1);
            }
        }
        return row_info_[row_id].row; // rows_[row_info_[row_id].index];
    }

    RowInfo *Table::GetRowInfo(integer_t row_id)
    {
        if (0 <= row_id && row_id < row_info_.size())
        {
            return &row_info_[row_id];
        }
        else
        {
            Log::Error("Table::GetRowInfo: invalid row id: %d\n", row_id);
            return nullptr;
        }
    }

    void Table::Clear()
    {
        for (auto &row : rows_)
        {
            row_info_[row->RowId()].index = -1;
            row_info_[row->RowId()].row = nullptr;
            delete row;
        }
        rows_.clear();
        used_memory_size_ = 0;
    }

    int Table::ElementSize()
    {
        return element_size_;
    }

    TableIterator Table::Iterator()
    {
        return TableIterator(*this);
    }
}
