#ifdef MULTIVERSO_USE_MPI

#include "multiverso/net/mpi_net.h"

namespace multiverso {

namespace {
MPI_Datatype GetDataType(char*)   { return MPI_CHAR; }
MPI_Datatype GetDataType(int*)    { return MPI_INT; }
MPI_Datatype GetDataType(float*)  { return MPI_FLOAT; }
MPI_Datatype GetDataType(double*) { return MPI_DOUBLE; }
}

template <typename ElemType>
void MPINetWrapper::Allreduce(ElemType* data, size_t elem_count, int op) {
  MPI_Allreduce(MPI_IN_PLACE, data, (int)elem_count,
    GetDataType(data), op, MPI_COMM_WORLD);
}

template void MPINetWrapper::Allreduce<char>(char*, size_t, int);
template void MPINetWrapper::Allreduce<int>(int*, size_t, int);
template void MPINetWrapper::Allreduce<float>(float*, size_t, int);
template void MPINetWrapper::Allreduce<double>(double*, size_t, int);

}  // namespace multiverso

#endif