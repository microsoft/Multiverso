#include "multiverso/allocator.h"

#include "multiverso/util/log.h"

namespace multiverso {

#define UNIQLOCK(mutex_) std::unique_lock<std::mutex>lock(mutex_);
#define UNLOCK() lock.unlock();

inline FreeList::FreeList(size_t size) :
size_(size) {
  free_ = new MemoryBlock(size, this);
}

FreeList::~FreeList() {
  MemoryBlock*move = free_, *next;
  while (move) {
    next = move->next;
    delete move;
    move = next;
  }
}

inline char* FreeList::Pop() {
  UNIQLOCK(mutex_);
  if (free_ == nullptr) {
    free_ = new MemoryBlock(size_, this);
  }
  char* data = free_->data();
  free_ = free_->next;
  return data;
}

inline void FreeList::Push(MemoryBlock*block) {
  UNIQLOCK(mutex_);
  block->next = free_;
  free_ = block;
}

inline MemoryBlock::MemoryBlock(size_t size, FreeList* list) :
next(nullptr) {
  data_ = new char[size + header_size_];
  *(FreeList**)(data_) = list;
  *(MemoryBlock**)(data_ + g_pointer_size) = this;
}

MemoryBlock::~MemoryBlock() {
  delete[]data_;
}

inline void MemoryBlock::Unlink() {
  if ((--ref_) == 0) {
    (*(FreeList**)data_)->Push(this);
  }
}

inline char* MemoryBlock::data() {
  ++ref_;
  return data_ + header_size_;
}

inline void MemoryBlock::Link() {
  ++ref_;
}

char* Allocator::New(size_t size) {
  const static size_t t = ((size_t)(-1)) << 5;
  size = ((size & 31) ? ((size & t) + 32) : size);
  UNIQLOCK(mutex_);
  if (pools_[size] == nullptr) {
    pools_[size] = new FreeList(size);
  }
  UNLOCK();

  return pools_[size]->Pop();
}

void Allocator::Free(char *data) {
  (*(MemoryBlock**)(data - g_pointer_size))->Unlink();
}

void Allocator::Refer(char *data) {
  (*(MemoryBlock**)(data - g_pointer_size))->Link();
}

Allocator::~Allocator() {
  Log::Debug("~Allocator, final pool size: %d\n", pools_.size());
  for (auto i : pools_) {
    delete i.second;
  }
}

} // namespace multiverso 