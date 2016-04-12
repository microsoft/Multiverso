#ifndef MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_
#define MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_

#include "multiverso/updater/updater.h"

namespace multiverso {

template <typename T>
class AdagradUpdater : public Updater<T> {
public:
  explicit AdagradUpdater(size_t) {}
  void Update(size_t, T*, T*,
              UpdateOption*, size_t) override {
    // TODO(feiga)
  }
protected:
  // TODO(feiga): to implemented
};

}

#endif // MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_
