#include "multiverso/c_api.h"

#include "multiverso/multiverso.h"
#include "multiverso/table/array_table.h"
#include "multiverso/table/matrix_table.h"
#include "multiverso/util/log.h"
#include "multiverso/updater/updater.h"


extern "C" {
void MV_Init(int* argc, char* argv[]) {
  multiverso::MV_Init(argc, argv);
}

void MV_ShutDown(){
  multiverso::MV_ShutDown();
}

void MV_Barrier(){
  multiverso::MV_Barrier();
}

int MV_NumWorkers(){
  return multiverso::MV_NumWorkers();
}

int  MV_WorkerId(){
  return multiverso::MV_WorkerId();
}

int  MV_ServerId(){
  return multiverso::MV_ServerId();
}

// Array Table
void MV_NewArrayTable(int size, TableHandler* out) {
  *out = multiverso::MV_CreateTable(multiverso::ArrayTableOption<float>(size));
}

void MV_GetArrayTable(TableHandler handler, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::ArrayWorker<float>*>(handler);
  worker->Get(data, size);
}

void MV_AddArrayTable(TableHandler handler, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::ArrayWorker<float>*>(handler);
  worker->Add(data, size);
}

void MV_AddArrayTableOption(TableHandler handler, float* data, int size, float lr, float mom, float rho, float lambda) {
	auto worker = reinterpret_cast<multiverso::ArrayWorker<float>*>(handler);
	multiverso::AddOption option;
	option.set_worker_id(multiverso::MV_WorkerId());
	option.set_learning_rate(lr);
	option.set_momentum(mom);
	option.set_rho(rho);
	option.set_lambda(lambda);
	worker->Add(data, size, &option);
}

void MV_AddAsyncArrayTable(TableHandler handler, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::ArrayWorker<float>*>(handler);
  worker->AddAsync(data, size);
}

void MV_AddAsyncArrayTableOption(TableHandler handler, float* data, int size, float lr, float mom, float rho, float lambda) {
	auto worker = reinterpret_cast<multiverso::ArrayWorker<float>*>(handler);
	multiverso::AddOption option;
	option.set_worker_id(multiverso::MV_WorkerId());
	option.set_learning_rate(lr);
	option.set_momentum(mom);
	option.set_rho(rho);
	option.set_lambda(lambda);
	worker->AddAsync(data, size, &option);
}

// MatrixTable
void MV_NewMatrixTable(int num_row, int num_col, TableHandler* out) {
  *out = multiverso::MV_CreateTable(multiverso::MatrixTableOption<float>(num_row, num_col));
}

void MV_GetMatrixTableAll(TableHandler handler, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::MatrixWorkerTable<float>*>(handler);
  worker->Get(data, size);
}

void MV_AddMatrixTableAll(TableHandler handler, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::MatrixWorkerTable<float>*>(handler);
  worker->Add(data, size);
}

void MV_AddAsyncMatrixTableAll(TableHandler handler, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::MatrixWorkerTable<float>*>(handler);
  worker->AddAsync(data, size);
}

void MV_GetMatrixTableByRows(TableHandler handler, float* data, int size,
                             int row_ids[], int row_ids_n) {
  auto worker = reinterpret_cast<multiverso::MatrixWorkerTable<float>*>(handler);
  worker->Get(data, size, row_ids, row_ids_n);
}

void MV_AddMatrixTableByRows(TableHandler handler, float* data, int size,
                             int row_ids[], int row_ids_n) {
  auto worker = reinterpret_cast<multiverso::MatrixWorkerTable<float>*>(handler);
  worker->Add(data, size, row_ids, row_ids_n);
}

void MV_AddAsyncMatrixTableByRows(TableHandler handler, float* data, int size,
                             int row_ids[], int row_ids_n) {
  auto worker = reinterpret_cast<multiverso::MatrixWorkerTable<float>*>(handler);
  worker->AddAsync(data, size, row_ids, row_ids_n);
}

}
