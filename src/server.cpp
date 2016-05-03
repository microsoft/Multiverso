#include "multiverso/server.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "multiverso/actor.h"
#include "multiverso/dashboard.h"
#include "multiverso/multiverso.h"
#include "multiverso/table_interface.h"
#include "multiverso/io/io.h"
#include "multiverso/util/configure.h"
#include "multiverso/util/mt_queue.h"
#include "multiverso/zoo.h"


namespace multiverso {

MV_DEFINE_bool(sync, false, "sync or async");
MV_DEFINE_int(backup_worker_ratio, 0, "ratio% of backup workers, set 20 means 20%");

Server::Server() : Actor(actor::kServer) {
  RegisterHandler(MsgType::Request_Get, std::bind(
    &Server::ProcessGet, this, std::placeholders::_1));
  RegisterHandler(MsgType::Request_Add, std::bind(
    &Server::ProcessAdd, this, std::placeholders::_1));
}

int Server::RegisterTable(ServerTable* server_table) {
  int id = static_cast<int>(store_.size());
  store_.push_back(server_table);
  return id;
}

void Server::ProcessGet(MessagePtr& msg) {
  MONITOR_BEGIN(SERVER_PROCESS_GET);
  if (msg->data().size() != 0) {
    MessagePtr reply(msg->CreateReplyMessage());
    int table_id = msg->table_id();
    CHECK(table_id >= 0 && table_id < store_.size());
    store_[table_id]->ProcessGet(msg->data(), &reply->data());
    SendTo(actor::kCommunicator, reply);
  }
  MONITOR_END(SERVER_PROCESS_GET);
}

void Server::ProcessAdd(MessagePtr& msg) {
  MONITOR_BEGIN(SERVER_PROCESS_ADD)
  if (msg->data().size() != 0) {
    MessagePtr reply(msg->CreateReplyMessage());
    int table_id = msg->table_id();
    CHECK(table_id >= 0 && table_id < store_.size());
    store_[table_id]->ProcessAdd(msg->data());
    SendTo(actor::kCommunicator, reply);
  }
  MONITOR_END(SERVER_PROCESS_ADD)
}


// The Sync Server implement logic to support Sync SGD training
// The implementation assumes all the workers will call same number
// of Add and/or Get requests
// The server promise all workers i-th Get will get the same parameters
// If worker k has add delta to server j times when its i-th Get 
// then the server will return the parameter after all K 
// workers finished their j-th update

// TODO(feiga): to delete this, SyncServer is a special case for 
//              BackupWorkerSyncServer

//class SyncServer : public Server {
//public:
//  SyncServer() : Server() {
//    int num_worker = Zoo::Get()->num_workers();
//    worker_get_clocks_.reset(new VectorClock(num_worker));
//    worker_add_clocks_.reset(new VectorClock(num_worker));
//  }
//
//  // make some modification to suit to the sync server
//  // please not use in other place, may different with the general vector clock
//  class VectorClock {
//  public:
//    explicit VectorClock(int n) : 
//      local_clock_(n, 0), global_clock_(0), size_(0) {}
//
//    // Return true when all clock reach a same number
//    virtual bool Update(int i) {
//      ++local_clock_[i];
//      if (global_clock_ < *(std::min_element(std::begin(local_clock_),
//        std::end(local_clock_)))) {
//        ++global_clock_;
//        if (global_clock_ == *(std::max_element(std::begin(local_clock_),
//          std::end(local_clock_)))) {
//          return true;
//        }
//      }
//      return false;
//    }
//
//    std::string DebugString() {
//      std::string os = "global ";
//      os += std::to_string(global_clock_) + " local: ";
//      for (auto i : local_clock_) os += std::to_string(i) + " ";
//      return os;
//    }
//
//    int local_clock(int i) const { return local_clock_[i]; }
//    int global_clock() const { return global_clock_; }
//
//  protected:
//    std::vector<int> local_clock_;
//    int global_clock_;
//    int size_;
//  };
//protected:
//  void ProcessAdd(MessagePtr& msg) override {
//    // 1. Before add: cache faster worker
//    int worker = Zoo::Get()->rank_to_worker_id(msg->src());
//    if (worker_get_clocks_->local_clock(worker) >
//        worker_get_clocks_->global_clock()) {
//      msg_add_cache_.Push(msg);
//      return;
//    }
//    // 2. Process Add
//    Server::ProcessAdd(msg);
//    // 3. After add: process cached process get if necessary
//    if (worker_add_clocks_->Update(worker)) {
//      CHECK(msg_add_cache_.Empty());
//      while (!msg_get_cache_.Empty()) {
//        MessagePtr get_msg;
//        CHECK(msg_get_cache_.TryPop(get_msg));
//        int get_worker = Zoo::Get()->rank_to_worker_id(get_msg->src());
//        Server::ProcessGet(get_msg);
//        worker_get_clocks_->Update(get_worker);
//      }
//    }
//  }
//
//  void ProcessGet(MessagePtr& msg) override {
//    // 1. Before get: cache faster worker
//    int worker = Zoo::Get()->rank_to_worker_id(msg->src());
//    if (worker_add_clocks_->local_clock(worker) >
//        worker_add_clocks_->global_clock()) {
//      // Will wait for other worker finished Add
//      msg_get_cache_.Push(msg);
//      return;
//    }
//    // 2. Process Get
//    Server::ProcessGet(msg);
//    // 3. After get: process cached process add if necessary
//    if (worker_get_clocks_->Update(worker)) {
//      CHECK(msg_get_cache_.Empty());
//      while (!msg_add_cache_.Empty()) {
//        MessagePtr add_msg;
//        CHECK(msg_add_cache_.TryPop(add_msg));
//        int add_worker = Zoo::Get()->rank_to_worker_id(add_msg->src());
//        Server::ProcessAdd(add_msg);
//        worker_add_clocks_->Update(add_worker);
//      }
//    }
//  }
//
//private:
//  std::unique_ptr<VectorClock> worker_get_clocks_;
//  std::unique_ptr<VectorClock> worker_add_clocks_;
//
//  MtQueue<MessagePtr> msg_add_cache_;
//  MtQueue<MessagePtr> msg_get_cache_;
//};

class WithBackupSyncServer : public Server {
public:
  WithBackupSyncServer() : Server() {
    num_worker_ = Zoo::Get()->num_workers();
    double backup_ratio = (double)MV_CONFIG_backup_worker_ratio / 100;
    num_sync_worker_ = num_worker_ - 
                       static_cast<int>(backup_ratio * num_worker_);
    CHECK(num_sync_worker_ > 0 && num_sync_worker_ <= num_worker_);
    if (num_sync_worker_ == num_worker_) {
      Log::Info("No backup worker, using the sync mode\n");
    }
    Log::Info("Sync with backup worker start: num_sync_worker = %d," 
              "num_total_worker = %d\n", num_sync_worker_, num_worker_);
    worker_get_clocks_.reset(new VectorClock(num_worker_));
    worker_add_clocks_.reset(new VectorClock(
      num_worker_, num_worker_ - num_sync_worker_));
  }

  // make some modification to suit to the sync server
  // please not use in other place, may different with the general vector clock
  class VectorClock {
  public:
    VectorClock(int num_worker, int num_backup_worker = 0) :
      local_clock_(num_worker, 0), global_clock_(0), num_worker_(num_worker), 
      num_sync_worker_(num_worker - num_backup_worker), progress_(0) {}

    // Return true when global clock meet the sync condition
    // sync: all worker reach the same clock
    // backup-worker-sync: sync-workers reach the same clock
    virtual bool Update(int i) {
      if (local_clock_[i]++ == global_clock_) {
        ++progress_;
      }
      if (progress_ >= num_sync_worker_) {
        ++global_clock_;
        progress_ = 0;
        for (auto i : local_clock_) {
          if (i > global_clock_) ++progress_;
        }
        if (global_clock_ == *(std::max_element(std::begin(local_clock_),
          std::end(local_clock_)))) {
          return true;
        }
      }
      return false;
    }

    std::string DebugString() {
      std::string os = "global ";
      os += std::to_string(global_clock_) + " local: ";
      for (auto i : local_clock_) os += std::to_string(i) + " ";
      return os;
    }

    int local_clock(int i) const { return local_clock_[i]; }
    int global_clock() const { return global_clock_; }

  protected:
    std::vector<int> local_clock_;
    int global_clock_;
    int num_worker_;
    int num_sync_worker_;
    int progress_;
  };
protected:
  void ProcessAdd(MessagePtr& msg) override {
    // 1. Before add: cache faster worker
    int worker = Zoo::Get()->rank_to_worker_id(msg->src());
    if (worker_get_clocks_->local_clock(worker) >
      worker_get_clocks_->global_clock()) {
      msg_add_cache_.Push(msg);
      return;
    }
    // 2. Process Add
    if (worker_add_clocks_->local_clock(worker) >= 
        worker_add_clocks_->global_clock()) {
      Server::ProcessAdd(msg);
    }
    // 3. After add: process cached process get if necessary
    if (worker_add_clocks_->Update(worker)) {
      CHECK(msg_add_cache_.Empty());
      while (!msg_get_cache_.Empty()) {
        MessagePtr get_msg;
        CHECK(msg_get_cache_.TryPop(get_msg));
        int get_worker = Zoo::Get()->rank_to_worker_id(get_msg->src());
        Server::ProcessGet(get_msg);
        worker_get_clocks_->Update(get_worker);
      }
    }
  }

  void ProcessGet(MessagePtr& msg) override {
    // 1. Before get: cache faster worker
    int worker = Zoo::Get()->rank_to_worker_id(msg->src());
    if (worker_add_clocks_->local_clock(worker) >
      worker_add_clocks_->global_clock()) {
      // Will wait for other worker finished Add
      msg_get_cache_.Push(msg);
      return;
    }
    // 2. Process Get
    Server::ProcessGet(msg);
    // 3. After get: process cached process add if necessary
    if (worker_get_clocks_->Update(worker)) {
      CHECK(msg_get_cache_.Empty());
      while (!msg_add_cache_.Empty()) {
        MessagePtr add_msg;
        CHECK(msg_add_cache_.TryPop(add_msg));
        int add_worker = Zoo::Get()->rank_to_worker_id(add_msg->src());
        if (worker_add_clocks_->local_clock(add_worker) >=
          worker_add_clocks_->global_clock()) {
          Server::ProcessAdd(msg);
        };
        worker_add_clocks_->Update(add_worker);
      }
    }
  }

private:
  std::unique_ptr<VectorClock> worker_get_clocks_;
  std::unique_ptr<VectorClock> worker_add_clocks_;

  MtQueue<MessagePtr> msg_add_cache_;
  MtQueue<MessagePtr> msg_get_cache_;

  // num_worker_ - num_sync_worker_ = num_backup_worker_
  int num_sync_worker_;
  int num_worker_;
};

Server* Server::GetServer() {
  if (!MV_CONFIG_sync) {
    Log::Info("Create a async server\n");
    return new Server();
  }
  // if (MV_CONFIG_backup_worker_ratio > 0.0) {
  Log::Info("Create a sync server\n");
  return new WithBackupSyncServer();
  // }
}

}  // namespace multiverso
