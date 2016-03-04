#ifndef MULTIVERSO_MATRIX_TABLE_H_
#define MULTIVERSO_MATRIX_TABLE_H_

#include "multiverso/table_interface.h"
#include "multiverso/util/log.h"
#include "multiverso/zoo.h"

#include <vector>

namespace multiverso {

	template <typename T>
	class MatrixWorkerTable : public WorkerTable {
	public:
		explicit MatrixWorkerTable(int num_row, int num_col) : WorkerTable(), num_row_(num_row), num_col_(num_col) {
			data_ = nullptr;
			data_vec_ = nullptr;

			row_size_ = num_col * sizeof(T);
			get_reply_count_ = 0;

			num_server_ = Zoo::Get()->num_servers();
			//compute row offsets in all servers
			server_offsets_.push_back(0);
			int length = num_row / num_server_;
			for (int i = 1; i < num_server_; ++i) {
				server_offsets_.push_back(i * length);
			}
			server_offsets_.push_back(num_row);

			Log::Debug("worker %d create matrixTable with %d rows %d colums.\n", Zoo::Get()->rank(), num_row, num_col);
		}

		T* raw() { return data_; }

		std::vector<T*>* row_vec() { return data_vec_; }

		// get whole table
		// data is user-allocated memory
		void Get(T* data){
			data_ = data;
			int whole = -1;
			WorkerTable::Get(Blob(&whole, sizeof(int)));
			Log::Debug("worker %d getting whole table.\n", Zoo::Get()->rank());
		}

		// data is user-allocated memory
		void Get(std::vector<int>& row_ids, T* data) {
			data_ = data;
			WorkerTable::Get(Blob(&row_ids[0], sizeof(int)* row_ids.size()));
			Log::Debug("worker %d getting rows\n", Zoo::Get()->rank());
		}

		void Get(std::vector<int>& row_ids, std::vector<T*>& data_vec) {
			data_vec_ = &data_vec;
			for (int i = 0; i < row_ids.size(); ++i){
				row_index_[row_ids[i]] = i;
			}
			WorkerTable::Get(Blob(&row_ids[0], sizeof(int) * row_ids.size()));
			Log::Debug("worker %d getting rows\n", Zoo::Get()->rank());
		}

		// Add whole table
		void Add(T* data) {
			CHECK_NOTNULL(data);
			int whole = -1;
			WorkerTable::Add(Blob(&whole, sizeof(int)), Blob(data, row_size_ * num_row_));
			Log::Debug("worker %d adding whole table.\n", Zoo::Get()->rank());
		}

		/*
		// Add one row with row_id, add all with row_id = -1
		void Add(int row_id, std::vector<T*>* data_vec) {
			CHECK_NOTNULL(data_vec);

			if (row_id == -1){
				Blob whole_table = Blob(&row_id, sizeof(int));
				Blob values = Blob(num_row_ * row_size_);
				//copy each row
				int offset = 0;
				for (int i = 0; i < num_row_; ++i){
					memcpy(values.data() + offset, (*data_vec)[i], row_size_);
					offset += row_size_;
				}
				WorkerTable::Add(whole_table, values);
			}
			else{
				WorkerTable::Add(Blob(&row_id, sizeof(int)), Blob((*data_vec)[row_id], row_size_));
			}
			Log::Debug("worker %d adding row with id %d.\n", Zoo::Get()->rank(), row_id);
		}
		*/

		void Add(std::vector<int>& row_ids, T* data) {
			Blob ids_blob(&row_ids[0], sizeof(int)* row_ids.size());
			Blob data_blob(data, row_ids.size() * row_size_);
			WorkerTable::Add(ids_blob, data_blob);
			Log::Debug("worker %d adding rows\n", Zoo::Get()->rank());
		}

		void Add(std::vector<int>& row_ids, const std::vector<T*>& data_vec) {
			Blob ids_blob(&row_ids[0], sizeof(int)* row_ids.size());
			Blob data_blob(row_ids.size() * row_size_);
			//copy each row
			for (int i = 0; i < row_ids.size(); ++i){
				memcpy(data_blob.data() + i * row_size_, data_vec[i], row_size_);
			}
			WorkerTable::Add(ids_blob, data_blob);
			Log::Debug("worker %d adding rows\n", Zoo::Get()->rank());
		}

		int Partition(const std::vector<Blob>& kv,
			std::unordered_map<int, std::vector<Blob> >* out) override {
			CHECK(kv.size() == 1 || kv.size() == 2);
			CHECK_NOTNULL(out);

			//get all elements, only happends in data_
			if (kv[0].size<int>() == 1 && kv[0].As<int>(0) == -1){
				for (int i = 0; i < num_server_; ++i){
					(*out)[i].push_back(kv[0]);
				}
				if (kv.size() == 2){	//process add values
					for (int i = 0; i < num_server_; ++i){
						Blob blob(kv[1].data() + server_offsets_[i] * row_size_,
							(server_offsets_[i + 1] - server_offsets_[i]) * row_size_);
						(*out)[i].push_back(blob);
					}
				}
				else {
					CHECK(get_reply_count_ == 0);
					get_reply_count_ = static_cast<int>(out->size());
				}
				return static_cast<int>(out->size());
			}

			//count row number in each server
			Blob row_ids = kv[0];
			std::unordered_map<int, int> count;
			int num_row_each = num_row_ / num_server_;
			for (int i = 0; i < row_ids.size<int>(); ++i){
				int dst = row_ids.As<int>(i) / num_row_each;
				dst = (dst == num_server_ ? dst - 1 : dst);
				++count[dst];
			}
			for (auto& it : count) { // Allocate memory
				std::vector<Blob>& vec = (*out)[it.first];
				vec.push_back(Blob(it.second * sizeof(int)));
				if (kv.size() == 2) vec.push_back(Blob(it.second * row_size_));
			}
			count.clear();

			for (int i = 0; i < row_ids.size<int>(); ++i) {
				int dst = row_ids.As<int>(i) / num_row_each;
				dst = (dst == num_server_ ? dst - 1 : dst);
				(*out)[dst][0].As<int>(count[dst]) = row_ids.As<int>(i);
				if (kv.size() == 2){//copy add values
					memcpy(&((*out)[dst][1].As<T>(count[dst] * num_col_)), &(kv[1].As<T>(i * num_col_)), row_size_);
				}
				++count[dst];
			}

			if (kv.size() == 1){
				CHECK(get_reply_count_ == 0);
				get_reply_count_ = static_cast<int>(out->size());
			}
			return static_cast<int>(out->size());
		}

		void ProcessReplyGet(std::vector<Blob>& reply_data) override {
			CHECK(reply_data.size() == 2 || reply_data.size() == 3);//3 for get all rows

			Blob keys = reply_data[0], data = reply_data[1];

			//get all rows, only happen in data_
			if (keys.size<int>() == 1 && keys.As<int>(0) == -1) {
				CHECK(data_ != nullptr);
				int server_id = reply_data[2].As<int>();
				memcpy(data_ + server_offsets_[server_id] * num_col_, data.data(), data.size());
				if ((--get_reply_count_) == 0)	data_ = nullptr;	//in case of wrong 
				return;
			}

			CHECK(data.size() == keys.size<int>() * row_size_);
			int offset = 0;
			if (data_ != nullptr){
				for (int i = 0; i < keys.size<int>(); ++i) {
					memcpy(data_ + keys.As<int>(i) * num_col_, data.data() + offset, row_size_);
					offset += row_size_;
				}
				if ((--get_reply_count_) == 0)	data_ = nullptr;
			}
			else { //data_vec_
				CHECK(data_vec_ != nullptr);
				for (int i = 0; i < keys.size<int>(); ++i) {
					memcpy((*data_vec_)[row_index_[keys.As<int>(i)]], data.data() + offset, row_size_);
					offset += row_size_;
				}
				if ((--get_reply_count_) == 0){
					data_vec_ = nullptr;
					row_index_.clear();
				}
			}
		}

	private:
		T* data_; // not owned
		std::vector<T*>* data_vec_;	//not owned
		std::unordered_map<int, int> row_index_; //index of data with row id in data_vec_
		int get_reply_count_; //number of unprocessed get reply
		int num_row_;
		int num_col_;
		int row_size_; // = sizeof(T) * num_col_
		int num_server_;
		std::vector<int> server_offsets_; //row id offset
	};

	// TODO(feiga): rename. The name static is inherited from last version
	// The storage is a continuous large chunk of memory
	template <typename T>
	class MatrixServerTable : public ServerTable {
	public:
		explicit MatrixServerTable(int num_row, int num_col) : ServerTable(), num_col_(num_col) {
			server_id_ = Zoo::Get()->rank();

			int size = num_row / Zoo::Get()->num_servers();
			row_offset_ = size * Zoo::Get()->rank();
			if (server_id_ == Zoo::Get()->num_servers() - 1){
				size = num_row - row_offset_;
			}
			storage_.resize(size * num_col);

			Log::Debug("server %d create matrixTable with %d row %d colums of %d rows.\n", server_id_, num_row, num_col, size);
		}

		void ProcessAdd(const std::vector<Blob>& data) override {
#ifdef MULTIVERSO_USE_BLAS
			// MKL update
			Log::Fatal("Not implemented yet\n");
#else
			CHECK(data.size() == 2);
			Blob values = data[1], keys = data[0];
			// add all values
			if (keys.size<int>() == 1 && keys.As<int>() == -1){
				CHECK(storage_.size() == values.size<T>());
				for (int i = 0; i < storage_.size(); ++i){
					storage_[i] += values.As<T>(i);
				}
				Log::Debug("server %d adding all rows with row offset %d with %d rows\n", server_id_, row_offset_, storage_.size() / num_col_);
				return;
			}

			CHECK(values.size() == keys.size<int>() * sizeof(T)* num_col_);
			int offset_v = 0;
			for (int i = 0; i < keys.size<int>(); ++i) {
				int offset_s = (keys.As<int>(i) -row_offset_) * num_col_;
				for (int j = 0; j < num_col_; ++j){
					storage_[j + offset_s] += values.As<T>(offset_v + j);
				}
				offset_v += num_col_;
				Log::Debug("server %d adding row with id %d\n", server_id_, keys.As<int>(i));
			}
#endif
		}

		void ProcessGet(const std::vector<Blob>& data,
			std::vector<Blob>* result) override {
			CHECK(data.size() == 1);
			CHECK_NOTNULL(result);

			Blob keys = data[0];
			result->push_back(keys); // also push the key

			//get all rows
			if (keys.size<int>() == 1 && keys.As<int>() == -1){
				result->push_back(Blob(storage_.data(), sizeof(T)* storage_.size()));
				result->push_back(Blob(&server_id_, sizeof(int)));
				Log::Debug("server %d getting all rows with row offset %d with %d rows\n", server_id_, row_offset_, storage_.size() / num_col_);
				return;
			}


			result->push_back(Blob(keys.size<int>() * sizeof(T)* num_col_));
			Blob& vals = (*result)[1];
			int row_size = sizeof(T)* num_col_;
			int offset_v = 0;
			for (int i = 0; i < keys.size<int>(); ++i) {
				int offset_s = (keys.As<int>(i) -row_offset_) * num_col_;
				memcpy(&(vals.As<T>(offset_v)), &storage_[offset_s], row_size);
				offset_v += num_col_;
				Log::Debug("server %d getting row with id %d\n", server_id_, keys.As<int>(i));
			}
		}

	private:
		int server_id_;
		int num_col_;
		int row_offset_;
		std::vector<T> storage_;
	};
}

#endif // MULTIVERSO_MATRIX_TABLE_H_
