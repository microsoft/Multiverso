#ifndef WORDEMBEDDING_BLOCK_QUEUE_H_
#define WORDEMBEDDING_BLOCK_QUEUE_H_

#include <condition_variable>
#include <mutex>
#include <queue>

#include "data_block.h"

namespace wordembedding {

  class BlockQueue {
  public:
    void Push(DataBlock *data_block);

    DataBlock* Pop();

    int const GetQueueSize();

  private:
    std::queue <DataBlock *> queues_;
    std::mutex mtx_;
    std::condition_variable repo_not_empty_;
  };
}
#endif
