#include "multiverso/allocator.h"

#include "multiverso/util/log.h"
#include "multiverso/util/configure.h"

namespace multiverso {

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
  std::lock_guard<std::mutex> lock(mutex_);
  if (free_ == nullptr) {
    free_ = new MemoryBlock(size_, this);
  }
  char* data = free_->data();
  free_ = free_->next;
  return data;
}

inline void FreeList::Push(MemoryBlock*block) {
  std::lock_guard<std::mutex> lock(mutex_);
  block->next = free_;
  free_ = block;
}

inline MemoryBlock::MemoryBlock(size_t size, FreeList* list) :
next(nullptr) {
  data_ = (char*)malloc(size + header_size_);
  *(FreeList**)(data_) = list;
  *(MemoryBlock**)(data_ + g_pointer_size) = this;
}

MemoryBlock::~MemoryBlock() {
  free(data_);
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

char* SmartAllocator::Malloc(size_t size) {
  const static size_t t = ((size_t)(-1)) << 5;
  size = ((size & 31) ? ((size & t) + 32) : size);
  std::unique_lock<std::mutex> lock(mutex_);
  if (pools_[size] == nullptr) {
    pools_[size] = new FreeList(size);
  }
  lock.unlock();

  return pools_[size]->Pop();
}

void SmartAllocator::Free(char *data) {
  (*(MemoryBlock**)(data - g_pointer_size))->Unlink();
}

void SmartAllocator::Refer(char *data) {
  (*(MemoryBlock**)(data - g_pointer_size))->Link();
}

SmartAllocator::~SmartAllocator() {
  Log::Debug("~SmartAllocator, final pool size: %d\n", pools_.size());
  for (auto i : pools_) {
    delete i.second;
  }
}

char* Allocator::Malloc(size_t size) {
  char* data = (char*)malloc(size + header_size_);
  // record ref
  *(std::atomic<int>**)data = new std::atomic<int>(1);
  return data + header_size_;
}

void Allocator::Free(char* data) {
  data -= header_size_;
  if (--(**(std::atomic<int>**)data) == 0) {
    delete *(std::atomic<int>**)data;
    free(data);
  }
}

void Allocator::Refer(char* data) {
  ++(**(std::atomic<int>**)(data - header_size_));
}

MV_DEFINE_string(allocator_type, "smart", "use smart allocator by default");
Allocator* Allocator::Get() {
  if (MV_CONFIG_allocator_type == "smart") {
    return new SmartAllocator();
  }
  return new Allocator();
}

} // namespace multiverso 