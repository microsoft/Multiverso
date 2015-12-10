#ifndef MULTIVERSO_WAITER_H_
#define MULTIVERSO_WAITER_H_

#include <mutex>
#include <condition_variable>

namespace multiverso {

class Waiter {
 public:
  Waiter() : ready_(true) {}
  void Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this](){ return ready_; } );
  }
  void Notify() {
    std::unique_lock<std::mutex> lock(mutex_);
    ready_ = true;
    cv_.notify_one();
  };
  void Reset() {
    std::unique_lock<std::mutex> lock(mutex_);
    ready_ = false;
  }
 private:
  mutable std::mutex mutex_;
  std::condition_variable cv_;
  bool ready_;
};
}

#endif // MULTIVERSO_WAITER_H_
