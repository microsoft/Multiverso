#ifndef  INCLUDE_MULTIVERSO_UTIL_ASYNC_BUFFER_H_
#define  INCLUDE_MULTIVERSO_UTIL_ASYNC_BUFFER_H_

#include <multiverso\table_interface.h>
#include <multiverso\util\waiter.h>
#include <thread>

namespace multiverso {

template<typename BufferType = void*>
class ASyncBuffer{
 public:
    // Creates an async buffer
    // buffer1 and buffer2: buffers used to save data from server
    // fill_buffer_action: action to fill a given buffer.
    ASyncBuffer(BufferType& buffer0, BufferType& buffer1,
        std::function<void(BufferType&)> fill_buffer_action)
        : buffer0_{ buffer0 }, buffer1_{ buffer1 },
        fill_buffer_func_{ fill_buffer_action } {
        init();
    }

    // Returns the ready buffer.
    // This function also automatically starts to prefetch data 
    // for the other buffer.
    BufferType& get_ready_buffer() {
        if (thread_ == nullptr) {
            init();
        }

        ready_waiter_.Wait();
        auto ready_buffer = get_buffer_to_fill(current_task_);
        prefetch_next_async();
        return ready_buffer;
    }

    ~ASyncBuffer() {
        if (thread_ != nullptr) {
            stop_prefetch();
        }
    }

    // Stops prefetch and releases related resource
    void stop_prefetch() {
        ready_waiter_.Wait();
        current_task_ = STOP_THREAD;
        new_task_waiter_.Notify();
        thread_->join();
        thread_ = nullptr;
    }

 protected:
    enum TaskType {
        FILL_BUFFER0,
        FILL_BUFFER1,
        STOP_THREAD
    };

 protected:
    void init() {
        ready_waiter_.Reset(0);
        new_task_waiter_.Reset(1);
        current_task_ = FILL_BUFFER1;
        thread_ = new std::thread(&ASyncBuffer<BufferType>::fill_buffer_routine, this);
        prefetch_next_async();
    }

    void prefetch_next_async() {
        current_task_ = (current_task_ == FILL_BUFFER1) ? FILL_BUFFER0 : FILL_BUFFER1;
        ready_waiter_.Reset(1);
        new_task_waiter_.Notify();
    }

    BufferType& get_buffer_to_fill(TaskType task) {
        CHECK(task != STOP_THREAD);
        return task == FILL_BUFFER0 ? buffer0_ : buffer1_;
    }

 private:
    BufferType& buffer0_;
    BufferType& buffer1_;
    std::function<void(BufferType&)> fill_buffer_func_;
    Waiter ready_waiter_;
    Waiter new_task_waiter_;
    TaskType current_task_;
    std::thread * thread_;

 private:
    void fill_buffer_routine() {
        while (true) {
            new_task_waiter_.Wait();
            if (current_task_ == STOP_THREAD) {
                break;
            }

            fill_buffer_func_(get_buffer_to_fill(current_task_));
            ready_waiter_.Notify();
            new_task_waiter_.Reset(1);
        }
    }
};

} // namespace multiverso


#endif  // INCLUDE_MULTIVERSO_UTIL_ASYNC_BUFFER_H_
