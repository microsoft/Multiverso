#ifndef MULTIVERSO_TIMER_H_
#define MULTIVERSO_TIMER_H_

#include <chrono>

namespace multiverso {

class Timer {
public:
  Timer();

  // (Re)start the timer
  void Start();

  // Get elapsed milliseconds since last Timer::Start
  double elapse();

private:
  using TimePoint = std::chrono::high_resolution_clock::time_point; // = std::chrono::system_clock::time_point;
  TimePoint start_point_;
};

}

#endif //MULTIVERSO_TIMER_H_
