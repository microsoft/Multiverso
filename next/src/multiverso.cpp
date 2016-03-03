#include "multiverso/multiverso.h"

#include "multiverso/net.h"
#include "multiverso/zoo.h"

namespace multiverso {

void MV_Init(int* argc, char* argv[], int role) {
  Zoo::Get()->Start(argc, argv, role);
}

void MV_ShutDown(bool finalize_net) {
  Zoo::Get()->Stop(finalize_net);
}

void MV_Barrier() { Zoo::Get()->Barrier(); }

int  MV_Rank() { return Zoo::Get()->rank(); }

int  MV_Size() { return Zoo::Get()->size(); }

int  MV_Worker_Id() {
  return Zoo::Get()->worker_rank();
}
int  MV_Server_Id() {
  return Zoo::Get()->server_rank();
}

int  MV_Num_Workers() {
  return Zoo::Get()->num_workers();
}
int  MV_Num_Servers() {
  return Zoo::Get()->num_servers();
}

int  MV_Net_Bind(int rank, char* endpoint) {
  return NetInterface::Get()->Bind(rank, endpoint);
}

int  MV_Net_Connect(int* ranks, char* endpoints[], int size) {
  return NetInterface::Get()->Connect(ranks, endpoints, size);
}

}
