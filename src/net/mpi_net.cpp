#include "multiverso/net/mpi_net.h"

namespace multiverso {

template static void MPINetWrapper::Allreduce<char>(char*, size_t);
template static void MPINetWrapper::Allreduce<int>(int*, size_t);
template static void MPINetWrapper::Allreduce<float>(float*, size_t);
template static void MPINetWrapper::Allreduce<double>(double*, size_t);

}  // namespace multiverso
