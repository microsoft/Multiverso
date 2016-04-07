#include "multiverso/updater/updater.h"

#include "multiverso/updater/adagrad_updater.h"
#include "multiverso/updater/smooth_gradient_updater.h"
#include "multiverso/util/configure.h"

namespace multiverso {

MV_DEFINE_string(updater_type, "default", "multiverso server updater type");

template <typename T>
void Updater<T>::Update(size_t num_element, T* data, T* delta,
                        UpdateOption* option = nullptr, 
                        size_t offset = 0) {
  // parallelism with openmp
  // TODO(feiga): change the magic number 4 with some configurable env variable
  #pragma omp parallel for schedule(static) num_threads(4)
  for (int i = 0; i < num_element; ++i) {
    data[i + offset] += delta[i];
  }
}

// Gradient-based updater in only for numerical table
// For simple int table, just using simple updater
template<>
Updater<int>* Updater<int>::GetUpdater(size_t size) {
  return new Updater<int>();
}

template <typename T>
Updater<T>* Updater<T>::GetUpdater(size_t size) {
  std::string type = MV_CONFIG_updater_type;
  if (type == "adagrad") return new AdagradUpdater<T>(size);
  if (type == "smooth_gradient") return new SmoothGradientUpdater<T>(size);
  // Default: simple updater
  return new Updater<T>();
}



MV_INSTANTIATE_CLASS_WITH_BASE_TYPE(Updater);

}