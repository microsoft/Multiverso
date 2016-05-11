#include "multiverso/c_api.h"

#include "multiverso/multiverso.h"
#include "multiverso/table/array_table.h"
#include "multiverso/table/matrix_table.h"
#include "multiverso/util/log.h"


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

void MV_NumWorkers(){
  multiverso::MV_NumWorkers();
}

int  MV_WorkerId(){
  return multiverso::MV_WorkerId();
}

int  MV_ServerId(){
  return multiverso::MV_ServerId();
}

// Array Table
void MV_NewArrayTable(int size, TableHandler* out) {
  // TODO(feiga): solve the memory issue
  multiverso::ArrayServer<float>* server = 
    new multiverso::ArrayServer<float>(size);
  multiverso::ArrayWorker<float>* worker = 
    new multiverso::ArrayWorker<float>(size);
  CHECK_NOTNULL(server);
  *out = worker;
}

void MV_GetArrayTable(TableHandler handler, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::ArrayWorker<float>*>(handler);
  worker->Get(data, size);
}

void MV_AddArrayTable(TableHandler handler, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::ArrayWorker<float>*>(handler);
  worker->Add(data, size);
}


// MatrixTable
void MV_NewMatrixTable(int num_row, int num_col, TableHandler* out) {
  // TODO: solve the memory issue, when to release worker and server?
  multiverso::MatrixServerTable<float>* server = 
    new multiverso::MatrixServerTable<float>(num_row, num_col);
  multiverso::MatrixWorkerTable<float>* worker = 
    new multiverso::MatrixWorkerTable<float>(num_row, num_col);
  CHECK_NOTNULL(server);
  *out = worker;
}

void MV_GetMatrixTableAll(TableHandler handler, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::MatrixWorkerTable<float>*>(handler);
  worker->Get(data, size);
}

void MV_AddMatrixTableAll(TableHandler handler, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::MatrixWorkerTable<float>*>(handler);
  worker->Add(data, size);
}

void MV_GetMatrixTableByRows(TableHandler handler, int row_ids[],
                              int row_ids_n, int num_col,  float** data) {
  auto worker = reinterpret_cast<multiverso::MatrixWorkerTable<float>*>(handler);
  worker->Get(std::vector<multiverso::integer_t>(row_ids, row_ids + row_ids_n),
              std::vector<float*>(data, data + row_ids_n), num_col);
}

void MV_AddMatrixTableByRows(TableHandler handler, int row_ids[],
                              int row_ids_n, int num_col, float* data[]) {
  auto worker = reinterpret_cast<multiverso::MatrixWorkerTable<float>*>(handler);
  worker->Add(std::vector<multiverso::integer_t>(row_ids, row_ids + row_ids_n),
              std::vector<float*>(data, data + row_ids_n), num_col);
}

  //ArrayServerFloat newArrayServerFloat(int i) {
  //  return (ArrayServerFloat) new multiverso::ArrayServer<float>(i);
  //}

  //ArrayWorkerFloat newArrayWorkerFloat(int i) {
  //  return (ArrayWorkerFloat) new multiverso::ArrayWorker<float>(i);
  //}

  //void getArrayWorkerFloat(ArrayWorkerFloat awf, float* data, int size) {
  //  ((multiverso::ArrayWorker<float>*) awf)->Get(data, size);
  //}

  // void addArrayWorkerFloat(ArrayServerFloat awf, float* data, int size, struct AddOption *options) {
  //   multiverso::AddOption mvoptions;
  //   mvoptions.set_learning_rate(options->learning_rate);
  //   mvoptions.set_momentum(options->momentum);
  //   mvoptions.set_rho(options->rho);
  //   ((multiverso::ArrayWorker<float>*) awf)->Add(data, size, &mvoptions);
  // }
}
