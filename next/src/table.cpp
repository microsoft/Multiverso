#include "multiverso/table_interface.h"
#include "zoo.h"

namespace multiverso {

WorkerTable::WorkerTable() {
  table_id_ = Zoo::Get()->RegisterTable(this);
}

int WorkerTable::GetAsync(Blob& keys) {
  int id = msg_id_++;
  waitings_[id] = new Waiter();
  MessagePtr msg = std::make_unique<Message>();
  msg->set_src(Zoo::Get()->rank());
  msg->set_type(MsgType::Request_Get);
  msg->set_msg_id(id);
  msg->set_table_id(table_id_);
  msg->Push(keys);
  Zoo::Get()->Deliver(actor::kWorker, msg);
  return id;
}

int WorkerTable::AddAsync(Blob& keys, Blob& values) {
  int id = msg_id_++;
  waitings_[id] = new Waiter();
  MessagePtr msg = std::make_unique<Message>();
  msg->set_src(Zoo::Get()->rank());
  msg->set_type(MsgType::Request_Add);
  msg->set_msg_id(id);
  msg->set_table_id(table_id_);
  msg->Push(keys);
  msg->Push(values);
  Zoo::Get()->Deliver(actor::kWorker, msg);
  return id;
}

ServerTable::ServerTable() {
  Zoo::Get()->RegisterTable(this);
}

}