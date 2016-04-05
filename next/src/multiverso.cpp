#include "multiverso/multiverso.h"

#include "multiverso/dashboard.h"
#include "multiverso/net.h"
#include "multiverso/zoo.h"

namespace multiverso {

void MV_Init(int* argc, char* argv[], int role, bool restart, int store_each_k) {
  Zoo::Get()->Start(argc, argv, role, restart, store_each_k);
}

void MV_ShutDown(bool finalize_net) {
  Zoo::Get()->Stop(finalize_net);
}

void MV_Barrier(int iter) { Zoo::Get()->Barrier(iter); }

int  MV_Rank() { return Zoo::Get()->rank(); }

int  MV_Size() { return Zoo::Get()->size(); }

int  MV_WorkerId() {
  return Zoo::Get()->worker_rank();
}
int  MV_ServerId() {
  return Zoo::Get()->server_rank();
}

int  MV_NumWorkers() {
  return Zoo::Get()->num_workers();
}
int  MV_NumServers() {
  return Zoo::Get()->num_servers();
}

int  MV_WorkerIdToRank(int worker_id) {
  return Zoo::Get()->worker_id_to_rank(worker_id);
}

int  MV_ServerIdToRank(int server_id) {
  return Zoo::Get()->server_id_to_rank(server_id);
}

void MV_Dashboard() {
  Dashboard::Display();
}

int  MV_NetBind(int rank, char* endpoint) {
  return NetInterface::Get()->Bind(rank, endpoint);
}

int  MV_NetConnect(int* ranks, char* endpoints[], int size) {
  return NetInterface::Get()->Connect(ranks, endpoints, size);
}

int MV_LoadTable(const std::string& table_file_path){
  return Zoo::Get()->LoadTable(table_file_path);
}

}
