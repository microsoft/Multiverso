#ifndef MULTIVERSO_UPDATER_SMOOTH_GRADIENT_UPDATER_H_
#define MULTIVERSO_UPDATER_SMOOTH_GRADIENT_UPDATER_H_

#include "updater.h"

namespace multiverso {

template <typename T>
class SmoothGradientUpdater : public Updater<T> {
public:
  SmoothGradientUpdater(size_t size): size_(size) {
    smooth_gradient_.resize(size_);
  }
  void Update(size_t num_element, T*data, T*delta, 
              UpdateOption* option) override {
    for (size_t index = 0; index < num_element; ++index) {
      smooth_gradient_[index] = option->momentum() * smooth_gradient_[index] 
        + (1 - option->momentum()) * delta[index];
      data[index] += smooth_gradient_[index];
    }
  }
  ~SmoothGradientUpdater() { delete[] smooth_gradient_; }
protected:
  std::vector<T> smooth_gradient_;
  size_t size_;
};

}

#endif // MULTIVERSO_UPDATER_SMOOTH_GRADIENT_UPDATER_H_