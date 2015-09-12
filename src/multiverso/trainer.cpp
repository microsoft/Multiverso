#include "log.h"
#include "lock.h"
#include "barrier.h"
#include "aggregator.h"
#include "double_buffer.h"
#include "multiverso.h"
#include "trainer.h"

namespace multiverso
{
    std::atomic<int> TrainerBase::trainer_count_(0);
    std::mutex TrainerBase::mtx_;
    Barrier TrainerBase::barrier_(1);

    // The base constructor, assign a trainer id to the instance.
    TrainerBase::TrainerBase()
    {
        std::lock_guard<std::mutex> lock(mtx_);
        trainer_id_ = trainer_count_++;
        barrier_.ResetNumThreads(trainer_count_);
    }

    TrainerBase::~TrainerBase() {}

    // Returns a pointer to the specified table.
    Table *TrainerBase::GetTable(integer_t table_id)
    {
        std::vector<Table*> &tables = Multiverso::double_buffer_->WorkerBuffer();
        if (0 <= table_id && table_id < tables.size())
        {
            return tables[table_id];
        }
        else
        {
            Log::Error(
                "Rank=%d Trainer=%d: TrainerBase::GetTable: Invalid table id: %d\n",
                Multiverso::ProcessRank(), trainer_id_, table_id);
            return nullptr;
        }
    }

    // Returns a RowBase pointer to the specified row.
    RowBase *TrainerBase::GetRowPtr(integer_t table_id, integer_t row_id)
    {
        Table *table = GetTable(table_id);
        RowBase *ret = (table != nullptr) ? table->GetRow(row_id) : nullptr;
        if (ret == nullptr)
        {
            Log::Error(
                "Rank=%d Trainer=%d: TrainerBase::GetTable: Invalid table or row ids: %d %d",
                Multiverso::ProcessRank(), trainer_id_, table_id, row_id);
        }
        return ret;
    }

    // Adds a delta to the specified element.
    void TrainerBase::AddPtr(integer_t table_id, integer_t row_id, 
        integer_t col_id, void *delta)
    {
        // push update to aggregator
        Multiverso::aggregator_->Add(trainer_id_, table_id, row_id, col_id, 
            delta);

        // update local cache according to lock option
        if (Multiverso::lock_option_ == LockOption::LockFree)
        {
            GetRowPtr(table_id, row_id)->Add(col_id, delta);
        }
        else if (Multiverso::lock_option_ == LockOption::Locked)
        {
            Multiverso::lock_manager_->Lock(table_id + row_id);
            GetRowPtr(table_id, row_id)->Add(col_id, delta);
            Multiverso::lock_manager_->Unlock(table_id + row_id);
        }
    }

    // Adds a row delta to the specified row.
    void TrainerBase::Add(integer_t table_id, integer_t row_id, void *delta)
    {
        Table *table = GetTable(table_id);
        RowInfo *row_info = table->GetRowInfo(row_id);
        if (row_info->format == Format::Dense)
        {
            char *buffer = static_cast<char*>(delta);
            int64_t element_size = table->ElementSize();
            // push udpate to aggregator
            for (integer_t col_id = 0; col_id < row_info->capacity; ++col_id)
            {
                Multiverso::aggregator_->Add(trainer_id_, table_id, row_id, 
                    col_id, buffer + (element_size * col_id));
            }

            // update local cache according to lock option
            if (Multiverso::lock_option_ == LockOption::LockFree)
            {
                table->GetRow(row_id)->Add(delta);
            }
            else if (Multiverso::lock_option_ == LockOption::Locked)
            {
                Multiverso::lock_manager_->Lock(row_id);
                table->GetRow(row_id)->Add(delta);
                Multiverso::lock_manager_->Unlock(row_id);
            }
        }
        else
        {
            Log::Error(
                "Rank=%d Trainer=%d: TrainerBase::Add: Not support batch add \
for sparse row: Table=%d Row=%d.\n", 
                Multiverso::ProcessRank(), trainer_id_, table_id, row_id);
        }
    }

    // Sends a Clock message to aggregator and wait for response
    void TrainerBase::Clock()
    {
        // REMARK (feiyan): consider a better interface and include these two 
        //                  methods.
        Multiverso::aggregator_->Add(trainer_id_, 0, -2, 0, nullptr);
        Multiverso::aggregator_->Wait();
    }

    void TrainerBase::BeginIteration()
    {
        Multiverso::double_buffer_->Start(trainer_id_ + 1);
    }

    void TrainerBase::EndIteration()
    {
        // send a flush message to the aggregator
        Multiverso::aggregator_->Add(trainer_id_, 0, -1, 0, nullptr);
        Multiverso::double_buffer_->End(trainer_id_ + 1);
    }

    void TrainerBase::PushDataBlock(DataBlockBase *data_block)
    {
        
        data_queue_.Push(data_block);
        if (data_block->Type() != DataBlockType::BeginClock &&
            data_block->Type() != DataBlockType::EndClock)
        {
            Multiverso::data_tag_[trainer_id_] = false;
        }
    }

    void TrainerBase::Start()
    {
        trainer_thread_ = std::thread(&TrainerBase::StartThread, this);
    }

    void TrainerBase::Stop()
    {
        data_queue_.Exit();
        trainer_thread_.join();
    }

    void TrainerBase::StartThread()
    {
// REMARK: yes, we may consider providing CPU affinity feature to users.
//#if defined(_WIN32) || defined(_WIN64)
//        long long maskLL = 0;
//        maskLL |= (1LL << (trainer_id_));
//        DWORD_PTR mask = maskLL;
//        SetThreadAffinityMask(GetCurrentThread(), mask);
//#endif
        DataBlockBase *data_block = nullptr;
        if (!Multiverso::is_pipeline_)
        {
            Multiverso::pipeline_barrier_->Wait();
        }
        while (data_queue_.Pop(data_block))
        {
            if (!Multiverso::is_pipeline_)
            {
                Multiverso::pipeline_barrier_->Wait();
            }
            switch (data_block->Type())
            {
            case DataBlockType::BeginClock:
                BeginClock();
                break;
            case DataBlockType::EndClock:
                Clock();
                EndClock();
                break;
            case DataBlockType::Test:
            case DataBlockType::Train:
                BeginIteration();
                TrainIteration(data_block);
                EndIteration();
            }
            if (barrier_.Wait())
            {
                data_block->IncreaseCount(-1);
            }
            if (data_queue_.Empty())
            {
                std::unique_lock<std::mutex> lock(Multiverso::mutex_);
                Multiverso::data_tag_[trainer_id_] = data_queue_.Empty();
                Multiverso::wait_cv_.notify_one();
            }
            if (!Multiverso::is_pipeline_)
            {
                Multiverso::pipeline_barrier_->Wait();
            }
        }
    }
}
