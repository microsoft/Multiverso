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

namespace multiverso {

Zoo::Zoo() {}

Zoo::~Zoo() {}

void Zoo::Start(int role) {
  Log::Debug("Zoo started\n");
  CHECK(role >= 0 && role <= 3);
  net_util_ = NetInterface::Get();
  net_util_->Init();
  nodes_.resize(size());
  nodes_[rank()].rank = rank();
  nodes_[rank()].role = role;
  // These actors can either reside in one process, or in different process
  // based on the configuration
  // For example, we can have a kind of configuration, N machines, each have a 
  // worker actor(thread), a server actor. Meanwhile, rank 0 node also servers 
  // as controller.
  // We can also have another configuration, N machine, rank 0 acts as 
  // controller, rank 1...M as workers(M < N), and rank M... N-1 as servers
  // All nodes have a communicator, and one(at least one) or more of other three 
  // kinds of actors
  Actor* communicator = new Communicator();
  if (rank() == 0)           Actor* controler = new Controller();
  if (node::is_worker(role)) Actor* worker = new Worker();
  if (node::is_server(role)) Actor* server = new Server();

  mailbox_.reset(new MtQueue<MessagePtr>);
  // Start all actors
  for (auto actor : zoo_) { actor.second->Start(); }
  // Init the network
  // activate the system
  RegisterNode();
  Log::Info("Rank %d: Zoo start sucessfully\n", rank());
}

void Zoo::Stop(bool finalize_net) {
  // Stop the system
  Barrier();

  // Stop all actors
  for (auto actor : zoo_) { actor.second->Stop(); }
  // Stop the network 
  if(finalize_net) net_util_->Finalize();
}

int Zoo::rank() const { return net_util_->rank(); }
int Zoo::size() const { return net_util_->size(); }

void Zoo::Deliver(const std::string& name, MessagePtr& msg) {
  CHECK(zoo_.find(name) != zoo_.end());
  zoo_[name]->Accept(msg);
}
void Zoo::Accept(MessagePtr& msg) {
  mailbox_->Push(msg);
}

void Zoo::RegisterNode() {
  MessagePtr msg(new Message()); //  = std::make_unique<Message>();
  msg->set_src(rank());
  msg->set_dst(0);
  msg->set_type(MsgType::Control_Register);
  msg->Push(Blob(&nodes_[rank()], sizeof(Node)));
  Deliver(actor::kCommunicator, msg);

  // waif for reply
  mailbox_->Pop(msg);
  CHECK(msg->type() == MsgType::Control_Reply_Register);
  Blob info_blob = msg->data()[0];
  Blob count_blob = msg->data()[1];
  num_workers_ = count_blob.As<int>(0);
  num_servers_ = count_blob.As<int>(1);
  CHECK(info_blob.size() == size() * sizeof(Node));
  memcpy(nodes_.data(), info_blob.data(), info_blob.size());
}

void Zoo::Barrier() {
  MessagePtr msg(new Message()); //  = std::make_unique<Message>();
  msg->set_src(rank());
  msg->set_dst(0); // rank 0 acts as the controller master. TODO(feiga):
  // consider a method to encapsulate this node information
  msg->set_type(MsgType::Control_Barrier);
  Deliver(actor::kCommunicator, msg);

  // Log::Debug("rank %d requested barrier.\n", rank());
  // wait for reply
  mailbox_->Pop(msg);
  CHECK(msg->type() == MsgType::Control_Reply_Barrier);
  // Log::Debug("rank %d reached barrier\n", rank());
}

int Zoo::RegisterTable(WorkerTable* worker_table) {
  return dynamic_cast<Worker*>(zoo_[actor::kWorker])
    ->RegisterTable(worker_table);
}

int Zoo::RegisterTable(ServerTable* server_table) {
  return dynamic_cast<Server*>(zoo_[actor::kServer])
    ->RegisterTable(server_table);
}

}
