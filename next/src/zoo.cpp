#include "multiverso/communicator.h"
#include "multiverso/zoo.h"
#include "multiverso/message.h"
#include "multiverso/actor.h"
#include "multiverso/util/mt_queue.h"
#include "multiverso/net.h"
#include "multiverso/util/log.h"
#include "multiverso/worker.h"
#include "multiverso/server.h"
#include "multiverso/controller.h"
#include "multiverso/dashboard.h"

namespace multiverso {

Zoo::Zoo() {}

Zoo::~Zoo() {}

void Zoo::Start(int* argc, char** argv, int role) {
  Log::Debug("Zoo started\n");
  CHECK(role >= 0 && role <= 3);
  // Init the network
  net_util_ = NetInterface::Get();
  net_util_->Init(argc, argv);
  nodes_.resize(size());
  nodes_[rank()].rank = rank();
  nodes_[rank()].role = role;
  mailbox_.reset(new MtQueue<MessagePtr>);

  // restart_ = restart;
  // store_each_k_ = store_each_k;

  // NOTE(feiga): the start order is non-trivial, communicator should be last.
  if (rank() == 0) { Actor* controler = new Controller(); controler->Start(); }
  if (node::is_server(role)) { Actor* server = new Server(); server->Start(); }
  if (node::is_worker(role)) { Actor* worker = new Worker(); worker->Start(); }
  Actor* communicator = new Communicator();
  communicator->Start();

  // activate the system
  RegisterNode();
  Log::Info("Rank %d: Zoo start sucessfully\n", rank());
}

void Zoo::Stop(bool finalize_net) {
  // Stop the system
  Barrier();

  Dashboard::Display();

  // Stop all actors
  for (auto actor : zoo_) { actor.second->Stop(); }
  // Stop the network 
  if(finalize_net) net_util_->Finalize();
}

int Zoo::rank() const { return NetInterface::Get()->rank(); }
int Zoo::size() const { return NetInterface::Get()->size(); }

void Zoo::SendTo(const std::string& name, MessagePtr& msg) {
  CHECK(zoo_.find(name) != zoo_.end());
  zoo_[name]->Receive(msg);
}
void Zoo::Receive(MessagePtr& msg) {
  mailbox_->Push(msg);
}

void Zoo::RegisterNode() {
  MessagePtr msg(new Message()); 
  msg->set_src(rank());
  msg->set_dst(0);
  msg->set_type(MsgType::Control_Register);
  msg->Push(Blob(&nodes_[rank()], sizeof(Node)));
  SendTo(actor::kCommunicator, msg);

  // waif for reply
  mailbox_->Pop(msg);
  CHECK(msg->type() == MsgType::Control_Reply_Register);
  Log::Debug("rank %d msg size %d\n", rank(), msg->data().size());
  CHECK(msg->data().size() == 2);
  Blob info_blob = msg->data()[0];
  Blob count_blob = msg->data()[1];
  Log::Debug("rank %d 1 %d\n", Zoo::Get()->rank(), count_blob.size<int>());
  num_workers_ = count_blob.As<int>(0);
  num_servers_ = count_blob.As<int>(1);
  Log::Debug("rank %d 2\n", Zoo::Get()->rank());
  worker_id_to_rank_.resize(num_workers_);
  server_id_to_rank_.resize(num_servers_);
  CHECK(info_blob.size() == size() * sizeof(Node));
  memcpy(nodes_.data(), info_blob.data(), info_blob.size());
  for (auto node : nodes_) {
    if (node.worker_id != -1) { 
      worker_id_to_rank_[node.worker_id] = node.rank; 
    }
    if (node.server_id != -1) {
      server_id_to_rank_[node.server_id] = node.rank;
    }
  }
  Log::Debug("rank %d end register\n", Zoo::Get()->rank());
}

void Zoo::Barrier() {
  MessagePtr msg(new Message()); 
  msg->set_src(rank());
  msg->set_dst(0); // rank 0 acts as the controller master. 
  // consider a method to encapsulate this node information
  msg->set_type(MsgType::Control_Barrier);
  SendTo(actor::kCommunicator, msg);

  Log::Debug("rank %d requested barrier.\n", rank());
  // wait for reply
  mailbox_->Pop(msg);
  CHECK(msg->type() == MsgType::Control_Reply_Barrier);
  Log::Debug("rank %d reached barrier\n", rank());
  
}

int Zoo::RegisterTable(WorkerTable* worker_table) {
  return dynamic_cast<Worker*>(zoo_[actor::kWorker])
    ->RegisterTable(worker_table);
}

int Zoo::RegisterTable(ServerTable* server_table) {
  return dynamic_cast<Server*>(zoo_[actor::kServer])
    ->RegisterTable(server_table);
}

//int Zoo::LoadTable(const std::string& table_file_path){
//  auto server = static_cast<Server*>(zoo_[actor::kServer]);
//  server->SetTableFilePath(table_file_path);
//  if (restart_){
//    return server->LoadTable(table_file_path);
//  }
//  return 0;
//}
}
