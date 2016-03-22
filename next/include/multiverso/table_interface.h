#ifndef MULTIVERSO_TABLE_INTERFACE_H_
#define MULTIVERSO_TABLE_INTERFACE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "multiverso/blob.h"
#include "multiverso/util/io.h"

namespace multiverso {

class Waiter;
// User implementent this
class WorkerTable {
public:
  WorkerTable();
  virtual ~WorkerTable() {}

  void Get(Blob keys) { Wait(GetAsync(keys)); }
  void Add(Blob keys, Blob values) { Wait(AddAsync(keys, values)); }

  // TODO(feiga): add call back
  int GetAsync(Blob keys); 
  int AddAsync(Blob keys, Blob values);

  void Wait(int id);

  void Reset(int msg_id, int num_wait);

  void Notify(int id);
  
  virtual int Partition(const std::vector<Blob>& kv,
    std::unordered_map<int, std::vector<Blob> >* out) = 0;

  virtual void ProcessReplyGet(std::vector<Blob>&) = 0;
  
  const std::string name() { return std::string(typeid(this).name());};

  // add user defined data structure
private:
  std::string table_name_;
  int table_id_;
  std::unordered_map<int, Waiter*> waitings_;
  int msg_id_;
};

// discribe the server parameter storage data structure and related method
class ServerTable {
public:
  ServerTable();
  virtual ~ServerTable() {}
  virtual void ProcessAdd(const std::vector<Blob>& data) = 0;
  virtual void ProcessGet(const std::vector<Blob>& data,
                          std::vector<Blob>* result) = 0;
  virtual void DumpTable(std::shared_ptr<Stream> os) = 0;
  virtual void RecoverTable(std::shared_ptr<Stream> in) = 0;

  const std::string name() const { return std::string(typeid(this).name());};
  
  // add user defined server process logic 
  void Process(const std::string instruction, const std::vector<Blob>& data, std::vector<Blob>* result = nullptr);

  // add user defined server storage data structure
};

// TODO(feiga): provide better table creator method
// Abstract Factory to create server and worker
//my new implementation
class TableFactory {
public:
  template<typename Key, typename Val = void>
  static WorkerTable* CreateTable(const std::string& table_type, const std::vector<void*>& table_args, 
    const std::string& dump_file_path = "");
  virtual ~TableFactory() {};
protected:
  virtual WorkerTable* CreateWorkerTable() = 0;
  virtual ServerTable* CreateServerTable() = 0;
};

//older one
class TableHelper {
public:
  TableHelper() {}
  WorkerTable* CreateTable();
  virtual ~TableHelper() {}

protected:
  virtual WorkerTable* CreateWorkerTable() = 0;
  virtual ServerTable* CreateServerTable() = 0;
};

//template function should be defined in the same file with declaration
template<typename Key, typename Val>
WorkerTable* TableFactory::CreateTable(const std::string& table_type, 
  const std::vector<void*>& table_args, const std::string& dump_file_path) {
  bool worker = MV_WorkerId() >= 0;
  bool server = MV_ServerId() >= 0;
  TableFactory* factory;
  if (table_type == "matrix") {
    factory = new MatrixTableFactory<Key>(table_args);
  }
  else if (table_type == "array") {
  }
  else CHECK(false);

  if (server) factory->CreateServerTable();
  if (worker) return factory->CreateWorkerTable();
  return nullptr;
}

}

#endif // MULTIVERSO_TABLE_INTERFACE_H_
