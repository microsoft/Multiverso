// Copyright 2015 Microsoft
#include "multiverso/table/sparse_matrix_table.h"
#include <vector>
#include <cctype>

#include "multiverso/multiverso.h"
#include "multiverso/util/log.h"
#include "multiverso/util/quantization_util.h"
#include "multiverso/updater/updater.h"

namespace multiverso {

bool split_rows = true;

// get whole table, data is user-allocated memory
template <typename T>
void SparseMatrixWorkerTable<T>::Get(T* data, size_t size,
  int worker_id) {
  CHECK(size == num_col_ * num_row_);
  int whole_table = -1;
  Get(whole_table, data, size, worker_id);
}

// data is user-allocated memory
template <typename T>
void SparseMatrixWorkerTable<T>::Get(int row_id, T* data, size_t size,
  int worker_id) {
  if (row_id >= 0) CHECK(size == num_col_);
  row_index_[row_id] = data;  // data_ = data;
  Blob keys(&row_id, sizeof(int) * 2);
  keys.As<int>(1) = worker_id;
  WorkerTable::Get(keys);
  Log::Debug("[Get] worker = %d, #row = %d\n", MV_Rank(), row_id);
}

template <typename T>
void SparseMatrixWorkerTable<T>::Get(const std::vector<int>& row_ids,
  const std::vector<T*>& data_vec, size_t size, int worker_id) {
  CHECK(size == num_col_);
  CHECK(row_ids.size() == data_vec.size());
  for (int i = 0; i < row_ids.size(); ++i) {
    row_index_[row_ids[i]] = data_vec[i];
  }
  Blob keys(row_ids.data(), sizeof(int)* (row_ids.size() + 1));
  keys.As<int>(row_ids.size()) = worker_id;
  WorkerTable::Get(keys);
  Log::Debug("[Get] worker = %d, #rows_set = %d\n", MV_Rank(),
    row_ids.size());
}

template <typename T>
int SparseMatrixWorkerTable<T>::Partition(const std::vector<Blob>& kv,
  std::unordered_map<int, std::vector<Blob>>* out) {
  int res;
  if (kv.size() == 1) {
    CHECK(kv.size() == 1 || kv.size() == 2);
    CHECK_NOTNULL(out);

    // last key is the worker_id
    size_t keys_size = kv[0].size<int>() - 1;
    int *keys = reinterpret_cast<int*>(kv[0].data());
    if (keys[0] == -1) {
      for (int i = 0; i < server_offsets_.size() - 1; ++i) {
        int rank = MV_ServerIdToRank(i);
        (*out)[rank].push_back(kv[0]);
      }

      CHECK(get_reply_count_ == 0);
      get_reply_count_ = static_cast<int>(out->size());
      res = static_cast<int>(out->size());
    } else {
      // count row number in each server
      std::unordered_map<int, int> count;
      std::vector<int> dest;
      int actual_num_server = static_cast<int>(server_offsets_.size() - 1);
      int num_row_each = num_row_ / actual_num_server;  //  num_server_;
      for (int i = 0; i < keys_size; ++i) {
        int dst = keys[i] / num_row_each;
        dst = (dst == actual_num_server ? dst - 1 : dst);
        dest.push_back(dst);
        ++count[dst];
      }

      // space for workder_id
      for (auto &kv : count) {
        ++kv.second;
      }

      for (auto& it : count) {  // Allocate memory
        int rank = MV_ServerIdToRank(it.first);
        std::vector<Blob>& vec = (*out)[rank];
        vec.push_back(Blob(it.second * sizeof(int)));
      }
      count.clear();

      //int offset = 0;
      for (int i = 0; i < keys_size; ++i) {
        int dst = dest[i];
        int rank = MV_ServerIdToRank(dst);
        (*out)[rank][0].As<int>(count[dst]) = keys[i];
        ++count[dst];
      }

      // append workder_id
      for (auto& kv : *out) {
        kv.second[0].As<int>(kv.second[0].size<int>() - 1) = keys[keys_size];
      }

      CHECK(get_reply_count_ == 0);
      get_reply_count_ = static_cast<int>(out->size());
      res = static_cast<int>(out->size());
    }
  } else {
    // call base class's Partition
    res = MatrixWorkerTable<T>::Partition(kv, out);
  }

  SparseFilter<T, int32_t>  filter(0);
  for (auto& pair : *out) {
    std::vector<Blob> comressed_blobs;
    filter.FilterIn(pair.second, &comressed_blobs);
    pair.second.swap(comressed_blobs);
  }

  return res;
}

template <typename T>
void SparseMatrixWorkerTable<T>::ProcessReplyGet(
  std::vector<Blob>& reply_data) {
  if (split_rows) {
    // replace row_index when original key == -1
    if (row_index_.size() == 1 && row_index_.find(-1) != row_index_.end()) {
      T* base_buf = row_index_[-1];
      size_t keys_size = reply_data[0].size<int>();
      int *keys = reinterpret_cast<int*>(reply_data[0].data());
      row_index_.clear();
      for (auto i = 0; i < keys_size; ++i) {
        row_index_[keys[i]] = base_buf + keys[i] * num_col_;
      }
    }
  }

  MatrixWorkerTable<T>::ProcessReplyGet(reply_data);
}

template <typename T>
SparseMatrixServerTable<T>::SparseMatrixServerTable(int num_row, int num_col,
  bool using_pipeline) : MatrixServerTable<T>(num_row, num_col) {
  auto server_count = multiverso::MV_Size();
  if (using_pipeline) {
    server_count *= 2;
  }

  for (auto i = 0; i < my_num_row_; ++i) {
    up_to_date_.push_back(std::move(std::vector<bool>(server_count, false)));
  }
}

template <typename T>
void SparseMatrixServerTable<T>::update_state_on_add(int worker_id,
  Blob keys_blob) {
  size_t keys_size = keys_blob.size<int>();
  int *keys = reinterpret_cast<int*>(keys_blob.data());
  // add all values
  if (keys_size == 1 && keys[0] == -1) {
    for (auto local_row_id = 0; local_row_id < my_num_row_; ++local_row_id) {
      for (auto id = 0; id < up_to_date_[local_row_id].size(); ++id) {
        up_to_date_[local_row_id][id] = (id == worker_id);
      }
    }
  } else {
    for (int i = 0; i < keys_size; ++i) {
      auto local_row_id = get_local_row_id(keys[i]);
      for (auto id = 0; id < up_to_date_[local_row_id].size(); ++id) {
        up_to_date_[local_row_id][id] = (id == worker_id);
      }
    }
  }
}

template <typename T>
void SparseMatrixServerTable<T>::update_state_on_get(int worker_id, int* keys,
  int key_size, std::vector<int>* out_rows) {

  if (worker_id == -1) {
    if (split_rows) {
      for (auto local_row_id = 0; local_row_id < my_num_row_; ++local_row_id)  {
        out_rows->push_back(get_global_row_id(local_row_id));
      }
    } else {
      out_rows->push_back(-1);
    }
  }

  // do not update flags for worker_id == -1
  if (worker_id == -1) {
    return;
  }

  if (key_size == 1 && keys[0] == -1) {
        if (split_rows) {
          for (auto local_row_id = 0; local_row_id < my_num_row_; ++local_row_id)  {
            if (!up_to_date_[local_row_id][worker_id]) {
              out_rows->push_back(get_global_row_id(local_row_id));
              up_to_date_[local_row_id][worker_id] = true;
            }
          }
        } else {
          out_rows->push_back(-1);
          for (auto local_row_id = 0; local_row_id < my_num_row_; ++local_row_id)  {
            if (!up_to_date_[local_row_id][worker_id]) {
              up_to_date_[local_row_id][worker_id] = true;
            }
          }
        }
  } else {
    for (auto i = 0; i < key_size; ++i)  {
      auto global_row_id = keys[i];
      auto local_row_id = get_local_row_id(global_row_id);
      if (!up_to_date_[local_row_id][worker_id]) {
        up_to_date_[local_row_id][worker_id] = true;
        out_rows->push_back(global_row_id);
      }
    }
  }

  // if all rows are up-to-date, then send the first row
  if (out_rows->size() == 0) {
    out_rows->push_back(get_global_row_id(0));
  }
}

template <typename T>
void SparseMatrixServerTable<T>::ProcessAdd(
  const std::vector<Blob>& compressed_data) {
  std::vector<Blob> data;
  SparseFilter<T, int32_t> filter(0);
  filter.FilterOut(compressed_data, &data);

  // must contain option that has worker id
  CHECK(data.size() == 3);
  UpdateOption option(data[2].data(), data[2].size());
  update_state_on_add(option.worker_id(), data[0]);
  MatrixServerTable<T>::ProcessAdd(data);
}

template <typename T>
void SparseMatrixServerTable<T>::ProcessGet(
  const std::vector<Blob>& compressed_data,
  std::vector<Blob>* result) {
  std::vector<Blob> data;
  SparseFilter<T, int32_t> filter(0);
  filter.FilterOut(compressed_data, &data);

  CHECK(data.size() == 1);
  CHECK_NOTNULL(result);

  size_t keys_size = data[0].size<int>() - 1;
  int *keys = reinterpret_cast<int*>(data[0].data());
  // get worker_id from at the last position
  auto workder_id = data[0].As<int>(keys_size);
  std::vector<int> outdate_rows;
#pragma warning( push )
#pragma warning( disable : 4267)
  update_state_on_get(workder_id, keys, keys_size, &outdate_rows);
#pragma warning( pop ) 
  Blob outdate_rows_blob(sizeof(int) * outdate_rows.size());
  for (auto i = 0; i < outdate_rows.size(); ++i) {
    outdate_rows_blob.As<int>(i) = outdate_rows[i];
  }

  std::vector<Blob> blos{ outdate_rows_blob };
  MatrixServerTable<T>::ProcessGet(blos, result);
}

MV_INSTANTIATE_CLASS_WITH_BASE_TYPE(SparseMatrixWorkerTable);
MV_INSTANTIATE_CLASS_WITH_BASE_TYPE(SparseMatrixServerTable);

}   //  namespace multiverso
