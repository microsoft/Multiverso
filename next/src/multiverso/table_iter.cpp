#include "row.h"
#include "table.h"
#include "table_iter.h"

namespace multiverso
{
    TableIterator::TableIterator(Table &table) :table_(table) { idx_ = 0; }
    TableIterator::~TableIterator() { }

    bool TableIterator::HasNext(){ return idx_ < table_.rows_.size(); }
    void TableIterator::Next(){ ++idx_; }

    integer_t TableIterator::RowId() { return table_.rows_[idx_]->RowId(); }
    RowBase* TableIterator::Row() { return table_.rows_[idx_]; }
}
