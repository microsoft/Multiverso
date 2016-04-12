#include "multiverso/updater/updater.h"

#include "multiverso/updater/adagrad_updater.h"
#include "multiverso/updater/smooth_gradient_updater.h"
#include "multiverso/updater/second_order_gradient_updater.h"
#include "multiverso/updater/sgd_updater.h"
#include "multiverso/util/configure.h"
#include "multiverso/util/log.h"

namespace multiverso {

MV_DEFINE_string(updater_type, "default", "multiverso server updater type");
MV_DEFINE_int(omp_threads, 4 , "#theads used by openMP for updater");

template <typename T>
void Updater<T>::Update(size_t num_element, T* data, T* delta,
                        UpdateOption*, size_t offset) {
  // parallelism with openMP
  #pragma omp parallel for schedule(static) num_threads(MV_CONFIG_omp_threads)
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
  if (type == "sgd") return new SGDUpdater<T>(size);
  if (type == "adagrad") return new AdaGradUpdater<T>(size);
  if (type == "smooth_gradient") return new SmoothGradientUpdater<T>(size);
  if (type == "second_order") return new SecondOrderUpdater<T>(size);
  // Default: simple updater
  return new Updater<T>();
}

MV_INSTANTIATE_CLASS_WITH_BASE_TYPE(Updater);

}