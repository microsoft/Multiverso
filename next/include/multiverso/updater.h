#ifndef MULTIVERSO_UPDATER_H_
#define MULTIVERSO_UPDATER_H_

namespace multiverso {

template <typename T>
class IUpdater {
public:
  virtual void Update(size_t num_element, T*data, T* delta) {
    for (int i = 0; i < num_element; ++i) {
      data[i] += delta[i];
    }
  }
};

template <typename T>
class SmoothGradientUpdater : public IUpdater<T> {
public:
  // cannot fit in the same interface ??
  virtual void Update(size_t num_element, T*data, T*delta, 
    T*smooth_gradient, T& smooth_momentum) {
    for (int i = 0; i < num_element; ++i) {
      smooth_gradient[i] = smooth_momentum * smooth_gradient[i] + (1 - smooth_momentum) * delta[i];
    }
    IUpdater<T>::Update(num_element, data, smooth_gradient);
  }
};

}

#endif //MULTIVERSO_UPDATER_H_