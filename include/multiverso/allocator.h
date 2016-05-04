#ifndef MULTIVERSO_ALLOCATOR_H_
#define MULTIVERSO_ALLOCATOR_H_

#include <map>

namespace multiverso {

const size_t g_pointer_size = sizeof(void*);

class MemoryBlock;
struct FreeList {
  FreeList(size_t size);
  ~FreeList();
  char *Pop();
  size_t size;
  MemoryBlock* free = nullptr;
};

class MemoryBlock {
public:
  MemoryBlock(size_t size, FreeList* list);
  ~MemoryBlock();
  inline char* data() { return data_ + header_size_; }
  MemoryBlock* next;
private:
  static const size_t header_size_ = (sizeof(MemoryBlock*) << 1);
  char* data_;
};

class Allocator {
public:
  char* New(size_t size);
  void Free(char* data);
  ~Allocator();
  inline Allocator* Get() const { return &allocator_; }
private:
  Allocator() = default;
  static Allocator allocator_;
  std::map<size_t, FreeList*> pools_;
};

} // namespace multiverso

#endif // MULTIVERSO_ALLOCATOR_H_