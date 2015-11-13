#ifndef MULTIVERSO_DATA_BLOCK_H_
#define MULTIVERSO_DATA_BLOCK_H_

#include <atomic>

namespace multiverso
{
    enum class DataBlockType : int
    {
        Train = 0,
        Test = 1,
        BeginClock = 2,
        EndClock = 3
    };

    class DataBlockBase
    {
    public:
        DataBlockBase();
        explicit DataBlockBase(DataBlockType type);
        DataBlockType Type() const { return type_; }
        void SetType(DataBlockType type) { type_ = type; }
        void IncreaseCount(int delta) { count_ += delta; }
        bool IsDone() { return count_ == 0; }

    private:
        DataBlockType type_;
        std::atomic<int> count_;
    };
}

#endif // MULTIVERSO_DATA_BLOCK_H_
