#include "multiverso/util/timer.h"

namespace multiverso {

Timer::Timer() {
  Start();
}

void Timer::Start() {
  start_point_ = std::chrono::high_resolution_clock::now();
}

double Timer::elapse() {
  TimePoint end_point = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> time_ms =
    end_point - start_point_;
  return time_ms.count();
}

}
