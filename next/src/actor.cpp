#include "multiverso/actor.h"
#include "multiverso/message.h"
#include "multiverso/util/mt_queue.h"
#include "multiverso/zoo.h"
#include "multiverso/util/log.h"

namespace multiverso {

Actor::Actor(const std::string& name) : name_(name) {
  Zoo::Get()->RegisterActor(name, this);
}

Actor::~Actor() {}

void Actor::Start() {
  mailbox_.reset(new MtQueue<MessagePtr>());
  thread_.reset(new std::thread(&Actor::Main, this));
}

void Actor::Stop() {
  while (!mailbox_->Empty()) { ; }
  mailbox_->Exit();
  thread_->join();
}

void Actor::Receive(MessagePtr& msg) { mailbox_->Push(msg); }

void Actor::Main() {
  Log::Debug("Rank %d: Start to run actor %s\n", Zoo::Get()->rank(), name().c_str());
  MessagePtr msg;
  while (mailbox_->Pop(msg)) {
    if (handlers_.find(msg->type()) != handlers_.end()) {
      handlers_[msg->type()](msg);
    }
    else if (handlers_.find(MsgType::Default) != handlers_.end()) {
      handlers_[MsgType::Default](msg);
    }
    else {
      CHECK(false); // Fatal error
    }
  }
}

void Actor::SendTo(const std::string& dst_name, MessagePtr& msg) {
  Zoo::Get()->SendTo(dst_name, msg);
}

}
