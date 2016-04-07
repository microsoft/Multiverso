//#ifndef MULTIVERSO_UPDATER_ASGD_UPDATER_H_
//#define MULTIVERSO_UPDATER_ASGD_UPDATER_H_
//
//#include "updater.h"
//
//namespace multiverso {
//template <typename T>
//class ASGDUpdater : public IUpdater<T> {
//public:
//  virtual void Update(size_t num_element, T*data, T* delta, AlgoOption* option = nullptr) override {
//    for (size_t i = 0; i < num_element; ++i)
//      data[i] += delta[i];
//  }
//  virtual ~ASGDUpdater(){}
//};
//}
//
//#endif // MULTIVERSO_UPDATER_ASGD_UPDATER_H_