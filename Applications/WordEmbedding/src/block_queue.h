#ifndef DISTRIBUTED_WORD_EMBEDDING_BLOCK_QUEUE_H_
#define DISTRIBUTED_WORD_EMBEDDING_BLOCK_QUEUE_H_

#include <condition_variable>
#include <mutex>
#include <queue>

#include "data_block.h"

namespace multiverso
{
	namespace wordembedding
	{
		class BlockQueue{
		public:
			std::queue <DataBlock *> queues;
			std::mutex mtx;
			std::condition_variable repo_not_empty;

			BlockQueue(){}
			~BlockQueue(){
				std::queue<DataBlock *>().swap(queues);
			}
		};
	}
}
#endif
