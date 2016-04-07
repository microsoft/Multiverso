#ifndef MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_
#define MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_

#include "multiverso/updater/updater.h"

namespace multiverso {

template <typename T>
class AdagradUpdater : public Updater<T> {
public:
  AdagradUpdater() {}
  void Update(size_t num_element, T* data, T* delta, 
              UpdateOption* option) override {
    // TODO(feiga)
  }
protected:
  // TODO(feiga): to implemented
};

}

#endif // MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_
