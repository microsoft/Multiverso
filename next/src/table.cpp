#include "multiverso/table_interface.h"

#include "multiverso/util/log.h"
#include "multiverso/util/waiter.h"
#include "multiverso/zoo.h"

namespace multiverso {

WorkerTable::WorkerTable() {
  table_id_ = Zoo::Get()->RegisterTable(this);
}

ServerTable::ServerTable() {
  Zoo::Get()->RegisterTable(this);
}

int WorkerTable::GetAsync(Blob keys) {
  int id = msg_id_++;
  waitings_[id] = new Waiter();
  MessagePtr msg(new Message()); //  = std::make_unique<Message>();
  msg->set_src(Zoo::Get()->rank());
  msg->set_type(MsgType::Request_Get);
  msg->set_msg_id(id);
  msg->set_table_id(table_id_);
  msg->Push(keys);
  Zoo::Get()->SendTo(actor::kWorker, msg);
  return id;
}

int WorkerTable::AddAsync(Blob keys, Blob values) {
  int id = msg_id_++;
  waitings_[id] = new Waiter();
  MessagePtr msg(new Message()); //  = std::make_unique<Message>();
  msg->set_src(Zoo::Get()->rank());
  msg->set_type(MsgType::Request_Add);
  msg->set_msg_id(id);
  msg->set_table_id(table_id_);
  msg->Push(keys);
  msg->Push(values);
  Zoo::Get()->SendTo(actor::kWorker, msg);
  return id;
}

void WorkerTable::Wait(int id) {
  CHECK(waitings_.find(id) != waitings_.end());
  CHECK(waitings_[id] != nullptr);
  waitings_[id]->Wait();
  delete waitings_[id];
  waitings_[id] = nullptr;
}

void WorkerTable::Reset(int msg_id, int num_wait) {
  CHECK_NOTNULL(waitings_[msg_id]);
  waitings_[msg_id]->Reset(num_wait);
}

void WorkerTable::Notify(int id) { 
  CHECK_NOTNULL(waitings_[id]);
  waitings_[id]->Notify(); 
}

}
