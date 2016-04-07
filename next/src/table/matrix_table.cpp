#include "multiverso/table/matrix_table.h"

#include <vector>

#include "multiverso/multiverso.h"
#include "multiverso/util/log.h"
#include "multiverso/util/quantization_util.h"
#include "multiverso/updater/updater.h"

namespace multiverso {

template <typename T>
MatrixWorkerTable<T>::MatrixWorkerTable(int num_row, int num_col) :
  WorkerTable(), num_row_(num_row), num_col_(num_col) {
  row_size_ = num_col * sizeof(T);
  get_reply_count_ = 0;

  num_server_ = MV_NumServers();
  //compute row offsets in all servers
  server_offsets_.push_back(0);
  int length = num_row / num_server_;
  int offset = length;
  //for (int i = 1; i < num_server_; ++i) {
  //  server_offsets_.push_back(offset);
  //  offset += length;
  //}
  //server_offsets_.push_back(num_row);
  while (length > 0 && offset < num_row) {
    server_offsets_.push_back(offset);
    offset += length;
  }
  server_offsets_.push_back(num_row);

  Log::Debug("[Init] worker =  %d, type = matrixTable, size =  [ %d x %d ].\n",
    MV_Rank(), num_row, num_col);
}

template <typename T>
void MatrixWorkerTable<T>::Get(T* data, size_t size){
  CHECK(size == num_col_ * num_row_);
  int whole_table = -1;
  Get(whole_table, data, size);
}

template <typename T>
void MatrixWorkerTable<T>::Get(int row_id, T* data, size_t size) {
  if (row_id >= 0) CHECK(size == num_col_);
  row_index_[row_id] = data; // data_ = data;
  WorkerTable::Get(Blob(&row_id, sizeof(int)));
  Log::Debug("[Get] worker = %d, #row = %d\n", MV_Rank(), row_id);
}

template <typename T>
void MatrixWorkerTable<T>::Get(const std::vector<int>& row_ids,
  const std::vector<T*>& data_vec,
  size_t size) {
  CHECK(size == num_col_);
  CHECK(row_ids.size() == data_vec.size());
  for (int i = 0; i < row_ids.size(); ++i){
    row_index_[row_ids[i]] = data_vec[i];
  }
  WorkerTable::Get(Blob(row_ids.data(), sizeof(int)* row_ids.size()));
  Log::Debug("[Get] worker = %d, #rows_set = %d\n", MV_Rank(), row_ids.size());
}

template <typename T>
void MatrixWorkerTable<T>::Add(T* data, size_t size, const UpdateOption* option) {
  CHECK(size == num_col_ * num_row_);
  int whole_table = -1;
  Add(whole_table, data, size, option);
}

template <typename T>
void MatrixWorkerTable<T>::Add(int row_id, T* data, size_t size, 
                               const UpdateOption* option) {
  if (row_id >= 0) CHECK(size == num_col_);
  Blob ids_blob(&row_id, sizeof(int));
  Blob data_blob(data, size * sizeof(T));
  WorkerTable::Add(ids_blob, data_blob, option);
  Log::Debug("[Add] worker = %d, #row = %d\n", MV_Rank(), row_id);
}

template <typename T>
void MatrixWorkerTable<T>::Add(const std::vector<int>& row_ids,
                               const std::vector<T*>& data_vec,
                               size_t size,
                               const UpdateOption* option) {
  CHECK(size == num_col_);
  Blob ids_blob(&row_ids[0], sizeof(int)* row_ids.size());
  Blob data_blob(row_ids.size() * row_size_);
  //copy each row
  for (int i = 0; i < row_ids.size(); ++i){
    memcpy(data_blob.data() + i * row_size_, data_vec[i], row_size_);
  }
  WorkerTable::Add(ids_blob, data_blob, option);
  Log::Debug("[Add] worker = %d, #rows_set = %d\n", MV_Rank(), row_ids.size());
}

template <typename T>
int MatrixWorkerTable<T>::Partition(const std::vector<Blob>& kv,
  std::unordered_map<int, std::vector<Blob>>* out) {
  CHECK(kv.size() == 1 || kv.size() == 2);
  CHECK_NOTNULL(out);

  size_t keys_size = kv[0].size<int>();
  int *keys = reinterpret_cast<int*>(kv[0].data());
  if (keys_size == 1 && keys[0] == -1){
    for (int i = 0; i < server_offsets_.size() - 1; ++i){
      int rank = MV_ServerIdToRank(i);
      (*out)[rank].push_back(kv[0]);
    }
    if (kv.size() == 2){	//process add values
      for (int i = 0; i < server_offsets_.size() - 1; ++i){
        int rank = MV_ServerIdToRank(i);
        Blob blob(kv[1].data() + server_offsets_[i] * row_size_,
          (server_offsets_[i + 1] - server_offsets_[i]) * row_size_);
        (*out)[rank].push_back(blob);
      }
    }
    else {
      CHECK(get_reply_count_ == 0);
      get_reply_count_ = static_cast<int>(out->size());
    }
    return static_cast<int>(out->size());
  }

  //count row number in each server
  std::unordered_map<int, int> count;
  std::vector<int> dest;
  int actual_num_server = static_cast<int>(server_offsets_.size() - 1);
  int num_row_each = num_row_ / actual_num_server; //  num_server_;
  for (int i = 0; i < keys_size; ++i){
    int dst = keys[i] / num_row_each;
    dst = (dst == actual_num_server ? dst - 1 : dst);
    dest.push_back(dst);
    ++count[dst];
  }
  for (auto& it : count) { // Allocate memory
    int rank = MV_ServerIdToRank(it.first);
    std::vector<Blob>& vec = (*out)[rank];
    vec.push_back(Blob(it.second * sizeof(int)));
    if (kv.size() == 2) vec.push_back(Blob(it.second * row_size_));
  }
  count.clear();

  int offset = 0;
  for (int i = 0; i < keys_size; ++i) {
    int dst = dest[i];
    int rank = MV_ServerIdToRank(dst);
    (*out)[rank][0].As<int>(count[dst]) = keys[i];
    if (kv.size() == 2){ // copy add values
      memcpy(&((*out)[rank][1].As<T>(count[dst] * num_col_)),
        kv[1].data() + offset, row_size_);
      offset += row_size_;
    }
    ++count[dst];
  }

  if (kv.size() == 1){
    CHECK(get_reply_count_ == 0);
    get_reply_count_ = static_cast<int>(out->size());
  }
  return static_cast<int>(out->size());
}

template <typename T>
void MatrixWorkerTable<T>::ProcessReplyGet(std::vector<Blob>& reply_data) {
  CHECK(reply_data.size() == 2 || reply_data.size() == 3); //3 for get all rows

  size_t keys_size = reply_data[0].size<int>();
  int *keys = reinterpret_cast<int*>(reply_data[0].data());
  T *data = reinterpret_cast<T*>(reply_data[1].data());

  //get all rows, only happen in T*
  if (keys_size == 1 && keys[0] == -1) {
    int server_id = reply_data[2].As<int>();
    CHECK_NOTNULL(row_index_[-1]);
    CHECK(server_id < server_offsets_.size() - 1);
    memcpy(row_index_[-1] + server_offsets_[server_id] * num_col_,
      data, reply_data[1].size());
  }
  else {
    CHECK(reply_data[1].size() == keys_size * row_size_);
    int offset = 0;
    for (int i = 0; i < keys_size; ++i) {
      CHECK_NOTNULL(row_index_[keys[i]]);
      memcpy(row_index_[keys[i]], data + offset, row_size_);
      offset += num_col_;
    }
  }
  //in case of wrong operation to user data
  if (--get_reply_count_ == 0) { row_index_.clear(); }
}




template <typename T>
MatrixServerTable<T>::MatrixServerTable(int num_row, int num_col) :
  ServerTable(), num_col_(num_col) {

  server_id_ = MV_ServerId();
  CHECK(server_id_ != -1);

  int size = num_row / MV_NumServers();
  if (size > 0) {
    row_offset_ = size * server_id_; // Zoo::Get()->rank();
    if (server_id_ == MV_NumServers() - 1){
      size = num_row - row_offset_;
    }
  }
  else {
    size = server_id_ < num_row ? 1 : 0;
    row_offset_ = server_id_;
  }
  my_num_row_ = size;
  storage_.resize(size * num_col);
  updater_ = Updater<T>::GetUpdater(size * num_col);
  Log::Debug("[Init] Server =  %d, type = matrixTable, size =  [ %d x %d ], total =  [ %d x %d ].\n",
    server_id_, size, num_col, num_row, num_col);
}

template <typename T>
void MatrixServerTable<T>::ProcessAdd(const std::vector<Blob>& data) {
  CHECK(data.size() == 2 || data.size() == 3);
  size_t keys_size = data[0].size<int>();
  int *keys = reinterpret_cast<int*>(data[0].data());
  T *values = reinterpret_cast<T*>(data[1].data());
  UpdateOption* option = nullptr;
  if (data.size() == 3) {
    option = new UpdateOption(data[2].data(), data[2].size());
  }
  // add all values
  if (keys_size == 1 && keys[0] == -1){
    size_t ssize = storage_.size();
    CHECK(ssize == data[1].size<T>());
    // for (int i = 0; i < ssize; ++i){
    //   storage_[i] += values[i];
    // }
    updater_->Update(ssize, storage_.data(), values, option);
    Log::Debug("[ProcessAdd] Server = %d, adding rows offset = %d, #rows = %d\n",
      server_id_, row_offset_, ssize / num_col_);
  }
  else {
    CHECK(data[1].size() == keys_size * sizeof(T)* num_col_);

    int offset_v = 0;
    CHECK(storage_.size() >= keys_size * num_col_);
    for (int i = 0; i < keys_size; ++i) {
      int offset_s = (keys[i] - row_offset_) * num_col_;
      updater_->Update(num_col_, storage_.data(), values + offset_v, option, offset_s);
      offset_v += num_col_;
      // for (int j = 0; j < num_col_; ++j){
      //  storage_[offset_s++] += values[offset_v++];
      //}
      Log::Debug("[ProcessAdd] Server = %d, adding #row = %d\n",
        server_id_, keys[i]);
    }
  }
  delete option;
}

template <typename T>
void MatrixServerTable<T>::ProcessGet(const std::vector<Blob>& data,
  std::vector<Blob>* result) {
  CHECK(data.size() == 1);
  CHECK_NOTNULL(result);

  result->push_back(data[0]); // also push the key

  size_t keys_size = data[0].size<int>();
  int *keys = reinterpret_cast<int*>(data[0].data());

  //get all rows
  if (keys_size == 1 && keys[0] == -1){
    result->push_back(Blob(storage_.data(), sizeof(T)* storage_.size()));
    result->push_back(Blob(&server_id_, sizeof(int)));
    Log::Debug("[ProcessGet] Server = %d, getting rows offset = %d, #rows = %d\n",
      server_id_, row_offset_, storage_.size() / num_col_);
    return;
  }

  int row_size = sizeof(T)* num_col_;
  result->push_back(Blob(keys_size * row_size));
  T* vals = reinterpret_cast<T*>((*result)[1].data());
  int offset_v = 0;
  for (int i = 0; i < keys_size; ++i) {
    int offset_s = (keys[i] - row_offset_) * num_col_;
    memcpy(vals + offset_v, &storage_[offset_s], row_size);
    offset_v += num_col_;
    Log::Debug("[ProcessAdd] Server = %d, getting #row = %d\n",
      server_id_, keys[i]);
  }
}

template <typename T>
void MatrixServerTable<T>::Store(Stream* s) {
  s->Write(storage_.data(), storage_.size() * sizeof(T));
}

template <typename T>
void MatrixServerTable<T>::Load(Stream* s) {
  s->Read(storage_.data(), storage_.size() * sizeof(T));
}

MV_INSTANTIATE_CLASS_WITH_BASE_TYPE(MatrixWorkerTable);
MV_INSTANTIATE_CLASS_WITH_BASE_TYPE(MatrixServerTable);

}
