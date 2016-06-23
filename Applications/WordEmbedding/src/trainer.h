#ifndef DISTRIBUTED_WORD_EMBEDDING_TRAINER_H_
#define DISTRIBUTED_WORD_EMBEDDING_TRAINER_H_
/*!
* file trainer.h
* \brief Class Trainer trains the model by every trainiteration
*/

#include <thread>
#include <chrono>

#include "multiverso/multiverso.h"
#include "multiverso/updater/updater.h"
#include "multiverso/table/matrix_table.h"

#include "constant.h"
#include "util.h"
#include "huffman_encoder.h"
#include "word_embedding.h"
#include "data_block.h"
#include "memory_manager.h"

namespace multiverso
{
	namespace wordembedding
	{
		class WordEmbedding;
		extern std::string g_log_suffix;
		class Trainer{
		public:
			int64 word_count;
			Trainer(int trainer_id, Option *option,
				Dictionary* dictionary, WordEmbedding* WordEmbedding);
			/*!
			* /brief Train one datablock
			*/

			~Trainer();
			void TrainIteration(DataBlock * data_block);

		private:
			int process_count_;
			int process_id_;
			int trainer_id_;
			Option *option_;
			real *hidden_act_, *hidden_err_;
			WordEmbedding* WordEmbedding_;
			Dictionary* dictionary_;
			int train_count_;
			clock_t start_;

			//No copying allowed
			Trainer(const Trainer&);
			void operator=(const Trainer&);
		};
	}
}
#endif
