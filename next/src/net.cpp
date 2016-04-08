#include "multiverso/net.h"

#include <limits>
#include <mutex>
#include "multiverso/message.h"
#include "multiverso/util/log.h"

#include "multiverso/net/zmq_net.h"
#include "multiverso/net/mpi_net.h"

namespace multiverso {

NetInterface* NetInterface::Get() {
#ifdef MULTIVERSO_USE_ZMQ
  static ZMQNetWrapper net_impl;
  return &net_impl;
#else
// #ifdef MULTIVERSO_USE_MPI
  // Use MPI by default
  static MPINetWrapper net_impl;
  return &net_impl;
// #endif
#endif
}

}  // namespace multiverso
