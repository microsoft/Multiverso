#ifndef MULTIVERSO_UPDATER_H_
#define MULTIVERSO_UPDATER_H_

namespace multiverso {

struct AlgoOption{
  AlgoOption(){}
  float smooth_momentum = 0.5;
  size_t offset = 0;
};

template <typename T>
class IUpdater {
public:
  virtual void Update(size_t num_element, T*data, T* delta, const AlgoOption& option) = 0;
  virtual ~IUpdater(){}
};

template <typename T>
class ASGDUpdater : public IUpdater<T> {
public:
  virtual void Update(size_t num_element, T*data, T* delta, const AlgoOption& option = AlgoOption()) {
    int upper = num_element + option.offset;
    for (int i = option.offset; i < upper; ++i)
      data[i] += delta[i];
  }
  virtual ~ASGDUpdater(){}
};

template <typename T>
class SmoothGradientUpdater : public IUpdater<T> {
public:
  SmoothGradientUpdater(size_t size) {
    smooth_gradient_ = new T[size];
  }
  virtual void Update(size_t num_element, T*data, T*delta, const AlgoOption& option) {
    size_t index = option.offset;
    for (int i = 0; i < num_element; ++i) {
      smooth_gradient_[index] = option.smooth_momentum * smooth_gradient_[index] + (1 - option.smooth_momentum) * delta[i];
      data[i] += smooth_gradient_[index];
      ++index;
    }
  }
  virtual ~SmoothGradientUpdater() { delete smooth_gradient_; }
protected:
  T* smooth_gradient_;
};

}

#endif //MULTIVERSO_UPDATER_H_