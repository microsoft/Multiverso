#include "data_block.h"

namespace multiverso
{
    DataBlockBase::DataBlockBase()
    {
        type_ = DataBlockType::Train;
        count_ = 0;
    }

    DataBlockBase::DataBlockBase(DataBlockType type)
    {
        type_ = type;
        count_ = 0;
    }
}
