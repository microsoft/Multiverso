#ifdef MULTIVERSO_USE_ZMQ

#include <string.h>
#include <algorithm>

#include "allreduce_engine.h"
#include "net_allreduce.h"

namespace multiverso {


void AllreduceEngine::Init(const AllreduceNetWrapper* linkers) {
  linkers_ = linkers;
  rank_ = linkers_->rank();
  world_size_ = linkers_->size();
  bruck_map_ = linkers_->GetBruckMap();
  recursive_halving_map_ = linkers_->GetRecursiveHalfingMap();
  block_start_ = new int[world_size_];
  block_len_ = new int[world_size_];
  buffer_size_ = 1024 * 1024;
  buffer_ = new byte[buffer_size_];
}

AllreduceEngine::~AllreduceEngine() {
  if (block_start_ != nullptr) { delete[]block_start_; }
  if (block_len_ != nullptr) { delete[]block_len_; }
  if (buffer_ != nullptr) { delete[] buffer_; }
}

void AllreduceEngine::Allreduce(byte* input, int input_size, int type_size, byte* output, ReduceFunction reducer) {

  int count = input_size / type_size;
  //if small package or small count , do it by all gather.(reduce the communication times.)
  if (count < world_size_ || input_size < 4096) {
    AllreduceByAllGather(input, input_size, type_size, output, reducer);
    return;
  }
  //assign the blocks to every rank_s.
  int step = (count + world_size_ - 1) / world_size_;
  if (step < 1) {
    step = 1;
  }
  block_start_[0] = 0;
  for (int i = 0; i < world_size_ - 1; ++i) {
    block_len_[i] = step * type_size < input_size - block_start_[i] ? step * type_size : input_size - block_start_[i];
    block_start_[i + 1] = block_start_[i] + block_len_[i];
  }
  block_len_[world_size_ - 1] = input_size - block_start_[world_size_ - 1];
  //do reduce scatter
  ReduceScatter(input, input_size, type_size, block_start_, block_len_, output, reducer);
  //do all gather
  Allgather(output, input_size, block_start_, block_len_, output);
}

void AllreduceEngine::AllreduceByAllGather(byte* input, int input_size, int type_size, byte* output, ReduceFunction reducer) {
  //assign blocks
  int all_size = input_size * world_size_;
  block_start_[0] = 0;
  block_len_[0] = input_size;
  for (int i = 1; i < world_size_; ++i) {
    block_start_[i] = block_start_[i - 1] + block_len_[i - 1];
    block_len_[i] = input_size;
  }

  if (input_size*world_size_ > buffer_size_) {
    delete[] buffer_;
    buffer_size_ = input_size*world_size_;
    buffer_ = new byte[buffer_size_];
  }
  Allgather(input, all_size, block_start_, block_len_, buffer_);
  for (int i = 1; i < world_size_; ++i) {
    reducer(buffer_ + block_start_[i], buffer_ + block_start_[0], input_size);
  }
  std::memcpy(output, buffer_, input_size);
}

void AllreduceEngine::Allgather(byte* input, int send_size, int all_size, byte* output) {
  //assign blocks
  block_start_[0] = 0;
  block_len_[0] = send_size;
  for (int i = 1; i < world_size_; ++i) {
    block_start_[i] = block_start_[i - 1] + block_len_[i - 1];
    block_len_[i] = send_size;
  }
  Allgather(input, all_size, block_start_, block_len_, output);
}

void AllreduceEngine::Allgather(byte* input, int all_size, int* block_start, int* block_len, byte* output) {
  int write_ptr = 0;
  std::memcpy(output, input, block_len[rank_]);
  write_ptr += block_len[rank_];
  int accumulated_block = 1;
  for (int i = 0; i < bruck_map_.k; ++i) {
    //send
    int cur_block_size = (1 << i) < world_size_ - accumulated_block ? (1 << i) : world_size_ - accumulated_block;
    int target = bruck_map_.out_ranks[i];
    int send_len = 0;
    for (int j = 0; j < cur_block_size; ++j) {
      send_len += block_len[(rank_ + j) % world_size_];
    }
    linkers_->Send(target, output, 0, send_len);
    //rec
    int incoming = bruck_map_.in_ranks[i];
    int need_recv_cnt = 0;
    for (int j = 0; j < cur_block_size; ++j) {
      need_recv_cnt += block_len[(rank_ + accumulated_block + j) % world_size_];
    }
    linkers_->Receive(incoming, output, write_ptr, need_recv_cnt);
    write_ptr += need_recv_cnt;
    accumulated_block += cur_block_size;
  }
  //rotate right 
  std::reverse<byte*>(output, output + all_size);
  std::reverse<byte*>(output, output + block_start[rank_]);
  std::reverse<byte*>(output + block_start[rank_], output + all_size);
}

void AllreduceEngine::ReduceScatter(byte* input, int input_size, int type_size, int* block_start, int* block_len, byte* output, ReduceFunction reducer) {

  bool is_powerof_2 = (world_size_ & (world_size_ - 1)) == 0 ? true : false;
  if (!is_powerof_2) {
    if (recursive_halving_map_.type == RecursiveHalvingNodeType::SendNeighbor) {
      //send local data to neighbor first
      linkers_->Send(recursive_halving_map_.neighbor, input, 0, input_size);
    }
    else if (recursive_halving_map_.type == RecursiveHalvingNodeType::ReciveNeighbor) {
      //recieve neighbor data first
      int need_recv_cnt = input_size;
      linkers_->Receive(recursive_halving_map_.neighbor, output, 0, need_recv_cnt);
      reducer(output, input, input_size);
    }
  }
  //start recursive halfing
  if (recursive_halving_map_.type != RecursiveHalvingNodeType::SendNeighbor) {

    for (int i = 0; i < recursive_halving_map_.k; ++i) {
      int target = recursive_halving_map_.ranks[i];
      int send_block_start = recursive_halving_map_.send_block_start[i];
      int recv_block_start = recursive_halving_map_.recv_block_start[i];
      //send
      int send_size = 0;
      for (int j = 0; j < recursive_halving_map_.send_block_len[i]; ++j) {
        send_size += block_len[send_block_start + j];
      }
      linkers_->Send(target, input, block_start[send_block_start], send_size);
      //receive
      int need_recv_cnt = 0;
      for (int j = 0; j < recursive_halving_map_.recv_block_len[i]; ++j) {
        need_recv_cnt += block_len[recv_block_start + j];
      }
      linkers_->Receive(target, output, 0, need_recv_cnt);
      //reduce
      reducer(output, input + block_start[recv_block_start], need_recv_cnt);
    }
  }
  int my_reduce_block_idx = rank_;

  if (!is_powerof_2) {
    if (recursive_halving_map_.type == RecursiveHalvingNodeType::ReciveNeighbor) {
      //send result to neighbor
      linkers_->Send(recursive_halving_map_.neighbor, input, block_start[recursive_halving_map_.neighbor], block_len[recursive_halving_map_.neighbor]);
    }
    else if (recursive_halving_map_.type == RecursiveHalvingNodeType::SendNeighbor) {
      //receive result from neighbor
      int need_recv_cnt = block_len[my_reduce_block_idx];
      linkers_->Receive(recursive_halving_map_.neighbor, output, 0, need_recv_cnt);
      return;
    }
  }
  std::memcpy(output, input + block_start[my_reduce_block_idx], block_len[my_reduce_block_idx]);
}

}

#endif