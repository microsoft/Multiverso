#ifndef MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_
#define MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_

#include "multiverso/updater/updater.h"I

#include <vector>
#include <cmath>

namespace multiverso {

template <typename T>
class AdaGradUpdater : public Updater<T> {
public:
  explicit AdaGradUpdater(size_t size):
    e(1e-6) {  
    historic_g_sqr_.resize(MV_NumWorkers());
    for (auto s : historic_g_sqr_){
      s.resize(size);
    }
  }
  void Update(size_t num_element, T* data, T* delta, 
              UpdateOption* option, size_t offset) override {
    for (size_t index = 0; index < num_element; ++index) {

      historic_g_sqr_[option->worker_id()][index + offset] -=
        delta[index] * delta[index] / option->learning_rate() / 
        option->learning_rate();

      data[index + offset] -= option->rho() /
        std::sqrtf(historic_g_sqr_[option->worker_id()][index + offset] + e) *
        delta[index] / option->learning_rate();
    }
  }
protected:
    std::vector< std::vector<T>> historic_g_sqr_;
    float e;
};

}

#endif // MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_
