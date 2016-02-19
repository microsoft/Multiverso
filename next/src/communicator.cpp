#include "multiverso/communicator.h"

#include <memory>

#include "multiverso/zoo.h"
#include "multiverso/net.h"
#include "multiverso/util/log.h"
#include "multiverso/util/mt_queue.h"

namespace multiverso {

namespace message {
// TODO(feiga): refator the ugly statement
bool to_server(MsgType type) {
  return (static_cast<int>(type)) > 0 &&
         (static_cast<int>(type)) < 32;
}

bool to_worker(MsgType type)  { 
  return (static_cast<int>(type)) < 0  &&
         (static_cast<int>(type)) > -32;
}

bool to_controler(MsgType type) {
  return (static_cast<int>(type)) > 32;
}
}

Communicator::Communicator() : Actor(actor::kCommunicator) {
  RegisterTask(MsgType::Default, std::bind(
    &Communicator::ProcessMessage, this, std::placeholders::_1));
  net_util_ = NetInterface::Get();
}

void Communicator::Main() {
  // TODO(feiga): join the thread, make sure it exit properly
#ifdef MULTIVERSO_USE_ZMQ
  recv_thread_.reset(new std::thread(&Communicator::Communicate, this));
  Actor::Main();
#else 
#ifdef MULTIVERSO_USE_MPI 
  MessagePtr msg;
  while (mailbox_->Alive()) {
    while (mailbox_->TryPop(msg)) {
      ProcessMessage(msg);
    };
    size_t size = net_util_->Recv(&msg);
    if (size > 0) LocalForward(msg);
  }
#endif
#endif
}

void Communicator::ProcessMessage(MessagePtr& msg) {
  if (msg->dst() != net_util_->rank()) {
    Log::Debug("Send a msg from %d to %d, type = %d\n", msg->src(), msg->dst(), msg->type());
    net_util_->Send(msg);
    return;
  }
  LocalForward(msg);
}

void Communicator::Communicate() {
  while (true) { // TODO(feiga): should exit properly
    MessagePtr msg(new Message()); //  = std::make_unique<Message>();
    size_t size = net_util_->Recv(&msg);
    if (size > 0) {
      // a message received
      Log::Debug("Recv a msg from %d to %d, size = %d, type = %d\n", msg->src(), msg->dst(), msg->size(), msg->type());
      CHECK(msg->dst() == Zoo::Get()->rank());
      LocalForward(msg);
    }
  }
  Log::Info("Comm recv thread exit\n");
}

void Communicator::LocalForward(MessagePtr& msg) {
  CHECK(msg->dst() == Zoo::Get()->rank());
  if (message::to_server(msg->type())) {
    SendTo(actor::kServer, msg);
  } else if (message::to_worker(msg->type())) {
    SendTo(actor::kWorker, msg);
  } else if (message::to_controler(msg->type())) {
    SendTo(actor::kController, msg);
  } else {
    // Send back to the msg queue of zoo
    Zoo::Get()->Receive(msg);
  }
}

}
