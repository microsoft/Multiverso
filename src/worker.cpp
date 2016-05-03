#include "multiverso/worker.h"

#include <vector>

#include "multiverso/dashboard.h"
#include "multiverso/util/mt_queue.h"
#include "multiverso/zoo.h"

namespace multiverso {

Worker::Worker() : Actor(actor::kWorker) {
  RegisterHandler(MsgType::Request_Get, std::bind(
    &Worker::ProcessGet, this, std::placeholders::_1));
  RegisterHandler(MsgType::Request_Add, std::bind(
    &Worker::ProcessAdd, this, std::placeholders::_1));
  RegisterHandler(MsgType::Reply_Get, std::bind(
    &Worker::ProcessReplyGet, this, std::placeholders::_1));
  RegisterHandler(MsgType::Reply_Add, std::bind(
    &Worker::ProcessReplyAdd, this, std::placeholders::_1));
}

int Worker::RegisterTable(WorkerTable* worker_table) {
  CHECK_NOTNULL(worker_table);
  int id = static_cast<int>(cache_.size());
  cache_.push_back(worker_table);
  return id;
}

void Worker::ProcessGet(MessagePtr& msg) {
  MONITOR_BEGIN(WORKER_PROCESS_GET)
  int table_id = msg->table_id();
  int msg_id = msg->msg_id();
  std::unordered_map<int, std::vector<Blob>> partitioned_key;
  int num = cache_[table_id]->Partition(msg->data(), &partitioned_key);
  cache_[table_id]->Reset(msg_id, num);
  for (auto i = 0; i < Zoo::Get()->num_servers(); i++) {
  //for (auto& it : partitioned_key) {
    MessagePtr msg(new Message());
    msg->set_src(Zoo::Get()->rank());
    //msg->set_dst(it.first);
    msg->set_dst(i);
    msg->set_type(MsgType::Request_Get);
    msg->set_msg_id(msg_id);
    msg->set_table_id(table_id);
    //msg->set_data(it.second);
    if (partitioned_key.find(i) != partitioned_key.end())
      msg->set_data(partitioned_key[i]);
    SendTo(actor::kCommunicator, msg);
  }
  MONITOR_END(WORKER_PROCESS_GET)
}

void Worker::ProcessAdd(MessagePtr& msg) {
  MONITOR_BEGIN(WORKER_PROCESS_ADD)
  int table_id = msg->table_id();
  int msg_id = msg->msg_id();
  std::unordered_map<int, std::vector<Blob>> partitioned_kv;
  CHECK_NOTNULL(msg.get());
  CHECK(!msg->data().empty());
  int num = cache_[table_id]->Partition(msg->data(), &partitioned_kv);
  cache_[table_id]->Reset(msg_id, num);
  for (auto i = 0; i < Zoo::Get()->num_servers(); i++) {
  //for (auto& it : partitioned_kv) {
    MessagePtr msg(new Message());
    msg->set_src(Zoo::Get()->rank());
    //msg->set_dst(it.first);
    msg->set_dst(i);
    msg->set_type(MsgType::Request_Add);
    msg->set_msg_id(msg_id);
    msg->set_table_id(table_id);
    //msg->set_data(it.second);
    if (partitioned_kv.find(i) != partitioned_kv.end())
      msg->set_data(partitioned_kv[i]);
    SendTo(actor::kCommunicator, msg);
  }
  MONITOR_END(WORKER_PROCESS_ADD)
}

void Worker::ProcessReplyGet(MessagePtr& msg) {
  MONITOR_BEGIN(WORKER_PROCESS_REPLY_GET)
  int table_id = msg->table_id();
  cache_[table_id]->ProcessReplyGet(msg->data());
  cache_[table_id]->Notify(msg->msg_id());
  MONITOR_END(WORKER_PROCESS_REPLY_GET)
}

void Worker::ProcessReplyAdd(MessagePtr& msg) {
  cache_[msg->table_id()]->Notify(msg->msg_id());
}

}  // namespace multiverso
