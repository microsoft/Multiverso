#include "multiverso/allocator.h"

#include "multiverso/util/log.h"

namespace multiverso {

FreeList::FreeList(size_t size) {
  this->size = size;
  free = new MemoryBlock(size, this);
}

FreeList::~FreeList() {
  MemoryBlock*move = free, *next;
  while (move) {
    next = move->next;
    delete move;
    move = next;
  }
}

char* FreeList::Pop() {
  if (free == nullptr) {
    free = new MemoryBlock(size, this);
  }
  MemoryBlock* block = free;
  free = free->next;
  return block->data();
}

MemoryBlock::MemoryBlock(size_t size, FreeList* list) :
next(nullptr) {
  data_ = new char[size + header_size_];
  *(FreeList**)data_ = list;
  *(MemoryBlock**)(data_ + g_pointer_size) = this;
}

MemoryBlock::~MemoryBlock() {
  delete[]data_;
}

char* Allocator::New(size_t size) {
  if (pools_[size] == nullptr) {
    pools_[size] = new FreeList(size);
  }

  return pools_[size]->Pop();
}

void Allocator::Free(char *data) {
  MemoryBlock* block = *(MemoryBlock**)(data - g_pointer_size);
  FreeList* list = *(FreeList**)(data - (g_pointer_size<<1));

  block->next = list->free;
  list->free = block;
}

Allocator::~Allocator() {
  for (auto i : pools_) {
    delete i.second;
  }
}

} // namespace multiverso 