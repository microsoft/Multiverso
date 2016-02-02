#include "worker.h"
#include "mt_queue.h"
#include "zoo.h"

namespace multiverso {

Worker::Worker() : Actor(actor::kWorker) {
  using namespace std::placeholders;
  RegisterTask(MsgType::Request_Get, std::bind(&Worker::ProcessGet, this, _1));
  RegisterTask(MsgType::Request_Add, std::bind(&Worker::ProcessAdd, this, _1));
}

int Worker::RegisterTable(WorkerTable* worker_table) {
  CHECK_NOTNULL(worker_table);
  int id = static_cast<int>(cache_.size());
  cache_.push_back(worker_table);
  return id;
}

void Worker::ProcessGet(MessagePtr& msg) {
  int table_id = msg->table_id();
  int msg_id = msg->msg_id();
  std::unordered_map<int, std::vector<Blob>> partitioned_key;
  int num = cache_[table_id]->Partition(msg->data(), &partitioned_key);
  for (auto& it : partitioned_key) {
    MessagePtr msg = std::make_unique<Message>();
    msg->set_src(Zoo::Get()->rank());
    msg->set_dst(it.first);
    msg->set_type(MsgType::Request_Get);
    msg->set_msg_id(msg_id);
    msg->set_table_id(table_id);
    msg->set_data(it.second);
    DeliverTo(actor::kCommunicator, msg);
  }
  // TODO(feiga): stop and wait, this is a naive implementation
  for (int i = 0; i < num; ++i) {
    MessagePtr msg;
    CHECK(mailbox_->Pop(msg));
    CHECK(msg->type() == MsgType::Reply_Get);
    cache_[table_id]->ProcessReplyGet(msg->data());
  }
  cache_[table_id]->Notify(msg_id);
}

void Worker::ProcessAdd(MessagePtr& msg) {  
  int table_id = msg->table_id();
  int msg_id = msg->msg_id();
  std::unordered_map<int, std::vector<Blob>> partitioned_kv;
  CHECK_NOTNULL(msg.get());
  CHECK(!msg->data().empty());
  int num = cache_[table_id]->Partition(msg->data(), &partitioned_kv);
  for (auto& it : partitioned_kv) {
    MessagePtr msg = std::make_unique<Message>();
    msg->set_src(Zoo::Get()->rank());
    msg->set_dst(it.first);
    msg->set_type(MsgType::Request_Add);
    msg->set_msg_id(msg_id);
    msg->set_table_id(table_id);
    msg->set_data(it.second);
    DeliverTo(actor::kCommunicator, msg);
  }

  for (int i = 0; i < num; ++i) {
    MessagePtr msg;
    CHECK(mailbox_->Pop(msg));
    CHECK(msg->type() == MsgType::Reply_Add);
  }
  // some cv issue
  cache_[table_id]->Notify(msg_id);
}

}