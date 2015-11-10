#ifndef MULTIVERSO_BARRIER_H_
#define MULTIVERSO_BARRIER_H_

/*!
 * \file barrier.h
 * \brief Defines Barrier for multithreading.
 */

#include <thread>
#include <atomic>
#include <condition_variable>

namespace multiverso
{
    /*!
     * \brief A synchronization barrier, which enables multiple threads 
     *        to wait until all threads have all reached a particular 
     *        point of execution before any thread continues.
     */
    class Barrier
    {
    public:
        /*!
         * \brief Creates a Barrier instance.
         * \param num_threads Number of threads joining the barrier
         */
        explicit Barrier(int num_threads);

        /*! 
         * \brief Resets the number of threas (barrier size).
         * \param num_threads New barrier size
         * \return 0 on success, -1 if num_threads is invalid (not greater than 0)
         */
        int ResetNumThreads(int num_threads);

        /*!
         * \brief Synchronize participating threads at the barrier.
         * \return true if current thread is the last one, false otherwise
         */
        bool Wait();

    private:
        int barrier_size_;      // number of threads barrierred by Wait()
        int num_in_waiting_;    // number of threads in waiting
        std::condition_variable cond_;
        std::mutex mutex_;

        // No copying allowed
        Barrier(const Barrier&);
        void operator=(const Barrier&);
    };
}

#endif // MULTIVERSO_BARRIER_H_
