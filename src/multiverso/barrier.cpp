#include "log.h"
#include "barrier.h"

namespace multiverso
{
    Barrier::Barrier(int num_threads)
    {
        num_in_waiting_ = 0;
        barrier_size_ = 0;
        ResetNumThreads(num_threads);
    }

    // TODO and DISCUSSION: If the barrier object is in a waiting status,
    // changing the barrier size may cause confusion. This method should be
    // called in the empty status.
    int Barrier::ResetNumThreads(int num_threads)
    {
        if (num_threads > 0)
        {
            barrier_size_ = num_threads;
            return 0;
        }
        else
        {
            Log::Error("Invalid barrier size %d, reset ignored.\n", num_threads);
            return -1;
        }
    }

    bool Barrier::Wait()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if ((++num_in_waiting_) == barrier_size_)   // the last one comes
        {
            num_in_waiting_ = 0;
            cond_.notify_all();
            return true;
        }
        else
        {
            cond_.wait(lock);
            return false;
        }
    }
}
