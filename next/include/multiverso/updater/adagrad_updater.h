#ifndef MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_
#define MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_

#include "multiverso/updater/updater.h"I

#include <vector>

namespace multiverso {

template <typename T>
class AdaGradUpdater : public Updater<T> {
public:
  explicit AdaGradUpdater(size_t size) {}
  void Update(size_t num_element, T* data, T* delta, 
              UpdateOption* option, size_t offset) override {

  }
protected:
    std::vector< std::vector<T>> historic_g_sqr_;
};

}

#endif // MULTIVERSO_UPDATER_ADAGRAD_UPDATER_H_
