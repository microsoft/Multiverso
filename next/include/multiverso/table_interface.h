#ifndef MULTIVERSO_TABLE_INTERFACE_H_
#define MULTIVERSO_TABLE_INTERFACE_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "multiverso/blob.h"

namespace multiverso {

class Waiter;
// User implementent this
class WorkerTable {
public:
  WorkerTable();
  virtual ~WorkerTable() {}

  void Get(Blob keys);
  void Add(Blob keys, Blob values);

  int GetAsync(Blob keys); 
  int AddAsync(Blob keys, Blob values);

  void Wait(int id);

  void Reset(int msg_id, int num_wait);

  void Notify(int id);
  
  virtual int Partition(const std::vector<Blob>& kv,
    std::unordered_map<int, std::vector<Blob> >* out) = 0;

  virtual void ProcessReplyGet(std::vector<Blob>&) = 0;
  
  // add user defined data structure
private:
  std::string table_name_;
  int table_id_;
  std::unordered_map<int, Waiter*> waitings_;
  int msg_id_;
};

// TODO(feiga): move to a seperate file
class Stream;

// interface for checkpoint table
class Serializable {
public:
  virtual void Store(Stream* s) = 0;
  virtual void Load(Stream* s) = 0;
};

// discribe the server parameter storage data structure and related method
class ServerTable {
public:
  ServerTable();
  virtual ~ServerTable() {}
  virtual void ProcessAdd(const std::vector<Blob>& data) = 0;
  virtual void ProcessGet(const std::vector<Blob>& data,
                          std::vector<Blob>* result) = 0;
};

}

#endif // MULTIVERSO_TABLE_INTERFACE_H_
