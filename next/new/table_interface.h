#ifndef MULTIVERSO_TABLE_INTERFACE_H_
#define MULTIVERSO_TABLE_INTERFACE_H_

#include <atomic>
#include <string>
#include <unordered_map>

#include "blob.h"
#include "log.h"
#include "waiter.h"
#include "message.h"

namespace multiverso {

// User implementent this
class WorkerTable {
public:
  WorkerTable();
  virtual ~WorkerTable() {}

  void Get(Blob& keys) { Wait(GetAsync(keys)); }
  void Add(Blob& keys, Blob& values) { Wait(AddAsync(keys, values)); }

  // NOTE(feiga): currently the async interface still doesn't work
  int GetAsync(Blob& keys); 
  int AddAsync(Blob& keys, Blob& values);

  void Wait(int id) { 
    CHECK(waitings_.find(id) != waitings_.end());
    waitings_[id]->Wait(); 
    delete waitings_[id];
    waitings_[id] = nullptr;
  }

  void Notify(int id) { waitings_[id]->Notify(); }
  
  virtual int Partition(const std::vector<Blob>& kv,
    std::unordered_map<int, std::vector<Blob> >* out) = 0;

  virtual void ProcessReplyGet(std::vector<Blob>&) = 0;

  // add user defined data structure
private:
  int table_id_;
  std::mutex m_;
  std::unordered_map<int, Waiter*> waitings_;
  std::atomic_int msg_id_;
};

// discribe the server parameter storage data structure and related method
class ServerTable {
public:
  ServerTable();
  virtual void ProcessAdd(const std::vector<Blob>& data) = 0;
  virtual void ProcessGet(const std::vector<Blob>& data,
    std::vector<Blob>* result) = 0;

  // add user defined server storage data structure
};

// TODO(feiga): provide better table creator method
// Abstract Factory to create server and worker
class TableFactory {
  //  static TableFactory* GetTableFactory();
  virtual WorkerTable* CreateWorker() = 0;
  virtual ServerTable* CreateServer() = 0;
  static TableFactory* fatory_;
};

namespace table {

}

class TableBuilder {
public:
  TableBuilder& SetArribute(const std::string& name, const std::string& val);
  WorkerTable* WorkerTableBuild();
  ServerTable* ServerTableBuild();
private:
  std::unordered_map<std::string, std::string> params_;
};

}

#endif // MULTIVERSO_TABLE_INTERFACE_H_