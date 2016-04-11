#ifndef MULTIVERSO_UPDATER_UPDATER_H_
#define MULTIVERSO_UPDATER_UPDATER_H_

#include <cstring>
#include <multiverso/multiverso.h>

namespace multiverso {

struct UpdateOption {
public:
  // TODO(feiga): default value;
  UpdateOption() { data_[0].i = MV_WorkerId(); }

  UpdateOption(const char* data, size_t size) {
    CopyFrom(data, size);
  }

  float learning_rate() const { return data_[2].f; }
  void set_learning_rate(float lr) { data_[2].f = lr; }
  float momentum() const { return data_[1].f; }
  void set_momentum(float momentum) { data_[1].f = momentum; }
  int worker_id() const { return data_[0].i; }

  //rho and lambda are two coefficient used by other algorithms
  float rho() const { return data_[3].f; }
  void set_rho(float rho) { data_[3].f = rho; }
  float lambda() const { return data_[4].f; }
  void set_lambda(float lambda) { data_[4].f = lambda; }


  const char* data() const { return reinterpret_cast<const char*>(&data_[0]); }
  size_t size() const { return kSize * sizeof(InternalType); }
  void CopyFrom(const char* data, size_t size) { 
    memcpy(data_, data, size);
  }
private:
  static const size_t kSize = 5;
  // Option can be either int type or float, 
  // to make it easy to serialize and deserialize
  union InternalType{
    int i;
    float f;
  };

  // May add other option for future potential update algorithm
  // 0: src worker id
  // 1: learning rate
  // 2: momentum
  // 3: rho
  // 4: lambda
  // ...
  InternalType data_[kSize];
};

template <typename T>
class Updater {
public:
  virtual ~Updater() = default;
  // The updater will update the data with delta in following way
  // Add delta[0 : num_element) to data[offset : offset+num_element)
  // NOTE(feiga): please note the trick part here. (may need further discussion)
  // for index in range(0, num_element):
  //    Update data[index + offset] with delta[index], option, and the updater member
  // This is mainly for model sparse update consideration
  virtual void Update(size_t num_element, T* data, T* delta, 
                      UpdateOption* option = nullptr, size_t offset = 0);

  // Factory method to get the updater
  static Updater<T>* GetUpdater(size_t size = 0);
};

#define MV_INSTANTIATE_CLASS_WITH_REAL_TYPE(classname) \
  template class classname<float>;                     \
  template class classname<double>;

#define MV_INSTANTIATE_CLASS_WITH_BASE_TYPE(classname) \
  MV_INSTANTIATE_CLASS_WITH_REAL_TYPE(classname)       \
  template class classname<int>;                       


}

#endif // MULTIVERSO_UPDATER_UPDATER_H_
