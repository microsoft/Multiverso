#ifndef MULTIVERSO_DOUBLE_BUFFER_H_
#define MULTIVERSO_DOUBLE_BUFFER_H_

/*!
 * \brief Defines DoubleBuffer 
 * \author feiga
 */

#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "barrier.h"

namespace multiverso 
{
    /*!
     * \brief A concurrency DoubleBuffer shared by multiple worker threads and
     *        one IO thread. All threads should call Start(thread_id) method 
     *        before access and call End(thread_id) after access. Any operation
     *        outside the range between Start and End are undefined.
     * \tparam Container underlying buffer type that DoubleBuffer holds
     */
    template<class Container>
    class DoubleBuffer 
    {
    public:
        /*!
         * \brief Constructs a DoubleBuffer
         * \param num_threads number of worker threads
         * \param reader_buffer pointer to one buffer
         * \param writer_buffer pointer to another buffer
         */
        DoubleBuffer(int num_threads, Container* reader_buffer, 
            Container* writer_buffer);

        /*! \brief Exit the DoubleBuffer, awake all threads blocked */
        void Exit();

        /*!
         * \brief Start access of DoubleBuffer
         * \param thread_id
         */
        void Start(int thread_id);
        
        /*! 
         * \brief End access of DoubleBuffer
         * \param thread_id
         */
        void End(int thread_id);

        /*!
         * \brief Get the worker buffer, only worker threads shoud call this.
         *        Must be called between Start and End
         * \return reference of underlying buffer
         */
        Container& WorkerBuffer();

        /*!
         * \brief Get the IO buffer, only IO threads should call this
         *        Must be called between Start and End
         * \return reference underlying buffer
         */
        Container& IOBuffer();

    private:
        /*! \brief num of total threads that have access to this buffer */
        int num_threads_;
        /*! \brief underlying reader buffer */
        Container* reader_buffer_;
        /*! \brief underlying writer buffer */
        Container* writer_buffer_;

        std::vector<bool> ready_;
        std::mutex mutex_;
        std::condition_variable condition_;
        bool exit_;
        int counter_;

        // No copying allowed
        DoubleBuffer(const DoubleBuffer&);
        void operator=(const DoubleBuffer&);
    };

    /*!
     * \brief RAII class for Buffer access
     * \tparam Buf, DoubleBuffer<T> type, should have method Start and End
     */
    template<class Buf>
    class BufferGuard 
    {
    public:
        BufferGuard(Buf& buffer, int thread_id);
        ~BufferGuard();
    private:
        Buf& buffer_;
        int thread_id_;
    };

    template<class Container>
    DoubleBuffer<Container>::DoubleBuffer(int num_threads, 
        Container* reader_buffer, Container* writer_buffer)
        : num_threads_(num_threads + 1), counter_(num_threads)
    {
        reader_buffer_ = reader_buffer;
        writer_buffer_ = writer_buffer;
        ready_.resize(num_threads_); // plus one for IO thread.
        ready_[0] = true; // id 0 is io thread by default.
        exit_ = false;
    }

    template<class Container>
    void DoubleBuffer<Container>::Exit()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        exit_ = true;
        condition_.notify_one();
    }

    template<class Container>
    void DoubleBuffer<Container>::Start(int thread_id)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock,
            [&](){ return ready_[thread_id] == true || exit_ == true; });
    }

    template<class Container>
    void DoubleBuffer<Container>::End(int thread_id)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        ready_[thread_id] = false;
        if (++counter_ == num_threads_)
        {
            counter_ = 0;
            std::swap(reader_buffer_, writer_buffer_);
            std::fill(ready_.begin(), ready_.end(), true);
            condition_.notify_all();
        }
    }

    template<class Container>
    Container& DoubleBuffer<Container>::WorkerBuffer()
    {
        return *reader_buffer_;
    }

    template<class Container>
    Container& DoubleBuffer<Container>::IOBuffer()
    {
        return *writer_buffer_;
    }

    template<class Buf>
    BufferGuard<Buf>::BufferGuard(Buf& buffer, int thread_id)
        : buffer_(buffer), thread_id_(thread_id)
    {
        buffer_.Start(thread_id_);
    }

    template<class Buf>
    BufferGuard<Buf>::~BufferGuard()
    {
        buffer_.End(thread_id_);
    }
}

#endif //MULTIVERSO_DOUBLE_BUFFER_H_
