#ifndef MULTIVERSO_UPDATER_UPDATER_H_
#define MULTIVERSO_UPDATER_UPDATER_H_

#include <cstring>

namespace multiverso {

struct UpdateOption {
public:
  // TODO(feiga): default value;
  UpdateOption() { }

  UpdateOption(const char* data, size_t size) {
    CopyFrom(data, size);
  }

  float momentum() const { return data_[0].f; }
  void set_momentum(float momentum) { data_[0].f = momentum; }
  float learning_rate() const { return data_[1].f; }
  void set_learning_rate(float lr) { data_[1].f = lr; }

  const char* data() const { return reinterpret_cast<const char*>(&data_[0]); }
  size_t size() const { return kSize * sizeof(InternalType); }
  void CopyFrom(const char* data, size_t size) { 
    // CHECK(size = this->size());
    memcpy(data_, data, size);
  }
private:
  static const size_t kSize = 2;
  // to make it easy serialize and deserialize
  union InternalType{
    int i;
    float f;
  };
  InternalType data_[kSize];
};

template <typename T>
class Updater {
public:
  virtual ~Updater() = default;
  virtual void Update(size_t num_element, T* data, T* delta, 
                      UpdateOption* option = nullptr);

  // Factory method to get the updater
  static Updater<T>* GetUpdater();
};

}

#endif // MULTIVERSO_UPDATER_UPDATER_H_
