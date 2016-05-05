#ifndef MULTIVERSO_ALLOCATOR_H_
#define MULTIVERSO_ALLOCATOR_H_

#include <unordered_map>
#include <mutex>

namespace multiverso {

const size_t g_pointer_size = sizeof(void*);

class MemoryBlock;
class FreeList {
public:
  FreeList(size_t size);
  ~FreeList();
  char *Pop();
  void Push(MemoryBlock*);
  void Refer(MemoryBlock*);
private:
  MemoryBlock* free_ = nullptr;
  size_t size_;
  std::mutex mutex_;
};

class MemoryBlock {
public:
  MemoryBlock(size_t size, FreeList* list);
  ~MemoryBlock();
  char* data();
  bool Unlink();
  void Link();
  MemoryBlock* next;
private:
  char* data_;
  int ref_ = 0;
  static const size_t header_size_ = (sizeof(MemoryBlock*) << 1);
};

class Allocator {
public:
  char* New(size_t size);
  void Free(char* data);
  void Refer(char *data);
  ~Allocator();
  inline static Allocator* Get() { 
    static Allocator allo; 
    return &allo; 
  }
private:
  std::unordered_map<size_t, FreeList*> pools_;
  std::mutex mutex_;
};

} // namespace multiverso

#endif // MULTIVERSO_ALLOCATOR_H_