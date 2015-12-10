#include "arena.h"
#include "log.h"
#include <stdlib.h>

namespace multiverso {

static const size_t kDefaultInitialBlockSize = 1 << 20; // 1MB
static const size_t kDefaultBlockUnitSize    = 1 << 12; // 4KB

Arena::Arena() {
  initial_block_size_ = kDefaultInitialBlockSize;
  block_unit_size_ = kDefaultBlockUnitSize;

  initial_mem_ = AllocateNew(initial_block_size_);

  next_ptr_ = initial_mem_.memory;
  available_size_ = initial_mem_.size;

  total_size_ = initial_mem_.size;

  request_size_ = 0; 
  request_time_ = 0;
}

Arena::~Arena() {
  free(initial_mem_.memory);
  FreeBlocks();
}

void Arena::Reset() {
    Log::Info("in reset\n");
  FreeBlocks();
  Resize();
  next_ptr_ = initial_mem_.memory;
  available_size_ = initial_mem_.size;

  total_size_ = initial_mem_.size;
  request_size_ = 0;
  request_time_ = 0;
  waiter_.Notify();
}

char* Arena::AllocateFallback(size_t bytes) {
  if (bytes > block_unit_size_ / 4) {
    MemBlock block = AllocateNew(bytes);
    mem_blocks_.push_back(block);
    return block.memory;
  } else {
    MemBlock block = AllocateNew(block_unit_size_);
    mem_blocks_.push_back(block);
    next_ptr_ = block.memory + bytes;
    available_size_ = block_unit_size_ - bytes;
    return block.memory;
  }
}

void Arena::Resize() {
  block_unit_size_ = 10 * (request_size_ / request_time_);
  if (true) { // TODO(feiga): judge if it's necessary to re-alloc
    initial_block_size_ = request_size_ / 2;
    free(initial_mem_.memory);
    initial_mem_ = AllocateNew(initial_block_size_);
  }
}

// TODO(feiga): alignment
Arena::MemBlock Arena::AllocateNew(size_t bytes) {
  // Add log, since this is not efficient, and should be called as less as possible
  // Log(INFO) << "Allocate new ";
  MemBlock result;
  result.memory = reinterpret_cast<char*>(malloc(bytes));
  result.size   = bytes;
  total_size_ += bytes;
  return result;
}

void Arena::FreeBlocks() {
  for (auto m : mem_blocks_) delete m.memory;
  mem_blocks_.clear();
}


}
