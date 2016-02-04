#ifndef MULTIVERSO_STATIC_TABLE_H_
#define MULTIVERSO_STATIC_TABLE_H_

#include "multiverso/table_interface.h"
#include "multiverso/util/log.h"
#include "multiverso/zoo.h"

namespace multiverso {

// A distributed shared std::vector<T> table

template <typename T>
class ArrayWorker : public WorkerTable {
public:
  explicit ArrayWorker(size_t size) : WorkerTable(), size_(size) {
    table_.resize(size);
    num_server_ = Zoo::Get()->num_servers();
    server_offsets_.push_back(0);
    CHECK(size_ > Zoo::Get()->num_servers()); // not support too small size vector
    int length = static_cast<int>(size_) / Zoo::Get()->num_servers();
    for (int i = 1; i < Zoo::Get()->num_servers(); ++i) {
      server_offsets_.push_back(i * length); // may not balance
    }
    server_offsets_.push_back(size_);
  }

  std::vector<T>& raw() { return table_; }

  // Get all element
  void Get() {
    int all_key = -1;
    WorkerTable::Get(Blob(&all_key, sizeof(int))); 
  }

  // Add all element
  void Add(T* data, size_t size) {
    CHECK(size == table_.size());
    int all_key = -1;

    WorkerTable::Add(Blob(&all_key, sizeof(int)), Blob(data, size * sizeof(T)));
  }

  int Partition(const std::vector<Blob>& kv,
    std::unordered_map<int, std::vector<Blob> >* out) override {
    CHECK(kv.size() == 1 || kv.size() == 2);
    for (int i = 0; i < num_server_; ++i) (*out)[i].push_back(kv[0]);
    if (kv.size() == 2) {
      CHECK(kv[1].size() == table_.size() * sizeof(T));
      for (int i = 0; i < num_server_; ++i) {
        Blob blob(kv[1].data() + server_offsets_[i] * sizeof(T), 
          (server_offsets_[i + 1] - server_offsets_[i]) * sizeof(T));
        (*out)[i].push_back(blob);
      }
    }
    return num_server_;
  }

  void ProcessReplyGet(std::vector<Blob>& reply_data) override {
    CHECK(reply_data.size() == 2);
    int id = (reply_data[0]).As<int>();
    CHECK(reply_data[1].size<T>() == (server_offsets_[id+1] - server_offsets_[id]));

    memcpy(table_.data() + server_offsets_[id], //  * sizeof(T), 
      reply_data[1].data(), reply_data[1].size());
  }
  
private:
  std::vector<T> table_;
  size_t size_;
  int num_server_;
  std::vector<size_t> server_offsets_;
};

// TODO(feiga): rename. The name static is inherited from last version
// The storage is a continuous large chunk of memory
template <typename T>
class ArrayServer : public ServerTable {
public:
  explicit ArrayServer(size_t size) : ServerTable() {
    server_id_ = Zoo::Get()->rank();
    size_ = size / Zoo::Get()->size();
    if (server_id_ == Zoo::Get()->num_servers()-1) { // last server 
      size_ += size % Zoo::Get()->num_servers();
    }
    storage_.resize(size_);
  }

  void ProcessAdd(const std::vector<Blob>& data) override {
#ifdef MULTIVERSO_USE_BLAS
    // MKL update
    Log::Fatal("Not implemented yet\n");
#else
    Blob keys = data[0], values = data[1];
    CHECK(keys.size<int>() == 1 && keys.As<int>() == -1); // Always request whole table
    CHECK(values.size() == size_ * sizeof(T));
    for (int i = 0; i < size_; ++i) storage_[i] += values.As<T>(i);
#endif
  }

  void ProcessGet(const std::vector<Blob>& data,
    std::vector<Blob>* result) override {
    size_t key_size = data[0].size<int>();
    CHECK(key_size == 1 && data[0].As<int>() == -1); // Always request the whole table
    Blob key(sizeof(int)); key.As<int>() = server_id_;
    Blob value(storage_.data(), sizeof(T) * size_);
    result->push_back(key);
    result->push_back(value);
  }

private:
  int server_id_;
  // T* storage_;
  std::vector<T> storage_;
  size_t size_; // number of element with type T
};
}

#endif // MULTIVERSO_STATIC_TABLE_H_