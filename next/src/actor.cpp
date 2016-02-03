#include "actor.h"
#include "message.h"
#include "util/mt_queue.h"
#include "zoo.h"
#include "util/log.h"

namespace multiverso {

Actor::Actor(const std::string& name) : name_(name) {
  Zoo::Get()->Register(name, this);
} 

Actor::~Actor() {}

void Actor::Start() {
  mailbox_.reset(new MtQueue<MessagePtr>());
  thread_.reset(new std::thread(&Actor::Main, this));
}

void Actor::Stop() {
  while (!mailbox_->Empty()) {;}
  mailbox_->Exit();
  thread_->join();
}

void Actor::Accept(MessagePtr& msg) { mailbox_->Push(msg); }

void Actor::Main() {
  // Log::Info("Start to run actor %s\n", name().c_str());
  MessagePtr msg;
  while (mailbox_->Pop(msg)) {
    if (handlers_.find(msg->type()) != handlers_.end()) {
      handlers_[msg->type()](msg);
    } else if (handlers_.find(MsgType::Default) != handlers_.end()) {
      handlers_[MsgType::Default](msg);
    } else {
      CHECK(false); // Fatal error
    }
  }
}

void Actor::DeliverTo(const std::string& dst_name, MessagePtr& msg) {
  Zoo::Get()->Deliver(dst_name, msg);
}

}