#include "data_block.h"

namespace multiverso
{
	namespace wordembedding
	{
		DataBlock::~DataBlock()
		{
			ClearSentences();
			ClearParameters();
		}

		size_t DataBlock::Size()
		{
			return sentences_.size();
		}

		//Add a new sentence to the DataBlock
		void DataBlock::AddSentence(int *head, int sentence_length,
			int64 word_count, uint64 next_random)
		{
			Sentence sentence(head, sentence_length, word_count, next_random);
			sentences_.push_back(sentence);
		}

		//Get the information of the index-th sentence
		void DataBlock::GetSentence(int index, int* &head,
			int &sentence_length, int64 &word_count, uint64 &next_random)
		{
			if (index >= 0 && index < sentences_.size())
			{
				sentences_[index].Get(head, sentence_length,
					word_count, next_random);
			}
			else
			{
				head = nullptr;
				sentence_length = 0;
				word_count = 0;
				next_random = 0;
			}
		}
		//Free the memory of sentences
		void DataBlock::ClearSentences()
		{
			for (int i = 0; i < sentences_.size(); ++i)
				delete[] sentences_[i].head;
			sentences_.clear();
		}

		void DataBlock::ClearParameters()
		{
			delete[] weight_IE_;
			delete[] weight_EO_;

			if (is_use_adagrad_)
			{
				delete []sum_gradient2_IE_;
				delete []sum_gradient2_EO_;
			}
		}
		
		//Set the weight of input-embedding vector
		void DataBlock::SetWeightIE(int input_node_id, real* ptr)
		{
			weight_IE_[input_node_id] = ptr;
		}

		//Set the weight of output-embedding vector
		void DataBlock::SetWeightEO(int output_node_id, real* ptr)
		{
			weight_EO_[output_node_id] = ptr;
		}
		//Get the weight of output-embedding vector
		real* DataBlock::GetWeightIE(int input_node_id)
		{
			return weight_IE_[input_node_id];
		}
		//Get the weight of output-embedding vector
		real* DataBlock::GetWeightEO(int output_node_id)
		{
			return weight_EO_[output_node_id];
		}

		void DataBlock::SetSumGradient2IE(int input_node_id, real* ptr)
		{
			sum_gradient2_IE_[input_node_id] = ptr;
		}

		//Set the weight of SumGradient-output vector
		void DataBlock::SetSumGradient2EO(int output_node_id, real* ptr)
		{
			sum_gradient2_EO_[output_node_id] = ptr;
		}

		//Get the weight of SumGradient-input vector
		real* DataBlock::GetSumGradient2IE(int input_node_id)
		{
			return sum_gradient2_IE_[input_node_id];
		}

		//Get the weight of SumGradient-output vector
		real* DataBlock::GetSumGradient2EO(int output_node_id)
		{
			return sum_gradient2_EO_[output_node_id];
		}

		void DataBlock::MallocMemory(int dictionary_size_, bool is_use_adagrad){
			weight_IE_ = new (std::nothrow)real*[dictionary_size_];
			assert(weight_IE_ != nullptr);
			weight_EO_ = new (std::nothrow)real*[dictionary_size_];
			assert(weight_EO_ != nullptr);
			is_use_adagrad_ = is_use_adagrad;

			if (is_use_adagrad_)
			{
				
				sum_gradient2_IE_ = new (std::nothrow)real*[dictionary_size_];
				sum_gradient2_EO_ = new (std::nothrow)real*[dictionary_size_];
				assert(sum_gradient2_IE_ != nullptr);
				assert(sum_gradient2_EO_ != nullptr);
			}
		}

		void DataBlock::PrintDataBlock(int embedding_size){
			std::vector<int> input_nodes(input_nodes.begin(), input_nodes.end());
			std::vector<int> output_nodes(output_nodes.begin(),output_nodes.end());

			for (int i = 0; i < input_nodes.size(); ++i)
			//for (int i = 0; i < 2; ++i)
			{
				real* ptr = GetWeightIE(input_nodes[i]);
				for (int j = 0; j < embedding_size; j++){
					std::cout << ptr[j] << " ";
				}
				std::cout << std::endl;
			}

			
			for (int i = 0; i < output_nodes.size(); ++i)
			{
				real* ptr = GetWeightEO(output_nodes[i]);
				for (int j = 0; j < embedding_size; j++){
					std::cout << ptr[j] << " ";
				}
				std::cout << std::endl;
			}
		}

		void DataBlock::SetLastFlag(){
			is_last_one_ = true;
		}

		bool DataBlock::isLast(){
			return is_last_one_;
		}
	}
}