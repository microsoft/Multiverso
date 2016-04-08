#ifndef MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_
#define MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_

#include "multiverso/updater/updater.h"

namespace multiverso {

template <typename T>
class AdagradUpdater : public Updater<T> {
public:
  explicit AdagradUpdater(size_t size) {}
  void Update(size_t num_element, T* data, T* delta, 
              UpdateOption* option, size_t offset) override {
    // TODO(feiga)
  }
protected:
  // TODO(feiga): to implemented
};

}

#endif // MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_
