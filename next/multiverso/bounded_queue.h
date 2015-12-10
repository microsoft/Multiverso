#ifndef MULTIVERSO_BOUND_QUEUE_H_
#define MULTIVERSO_BOUND_QUEUE_H_

#include "base.h"

namespace multiverso {
 
template <typename T, int N>
class BoundQueue {
public:
  void Push(T* item);
  void Pop(T* item);
  T& Front();
 
private:
  T queue_[N];
  index_t index_;
};

template <typename T>
typedef BoundQueue<T, 2> DoubleBuffer<T>;
}

#endif // MULTIVERSO_BOUND_QUEUE_H_