#ifndef MULTIVERSO_UPDATER_UPDATER_H_
#define MULTIVERSO_UPDATER_UPDATER_H_

namespace multiverso {

struct AlgoOption{
  AlgoOption(){}
  float momentum = 0.5;
};

template <typename T>
class IUpdater {
public:
  virtual void Update(size_t num_element, T*data, T* delta, AlgoOption* option = nullptr) = 0;
  virtual ~IUpdater(){}
};



}

#endif // MULTIVERSO_UPDATER_UPDATER_H_
