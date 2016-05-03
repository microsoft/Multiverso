#include "multiverso/c_api.h"

#include "multiverso/multiverso.h"
#include "multiverso/table/array_table.h"
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

void MV_NewTable(int size, TableHandle* out) {
  // TODO(feiga): solve the memory issue
  multiverso::ArrayServer<float>* server = 
    new multiverso::ArrayServer<float>(size);
  multiverso::ArrayWorker<float>* worker = 
    new multiverso::ArrayWorker<float>(size);
  CHECK_NOTNULL(server);
  *out = worker;
}

void MV_Get(TableHandle handle, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::ArrayWorker<float>*>(handle);
  worker->Get(data, size);
}

void MV_Add(TableHandle handle, float* data, int size) {
  auto worker = reinterpret_cast<multiverso::ArrayWorker<float>*>(handle);
  worker->Add(data, size);
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

  //void addArrayWorkerFloat(ArrayServerFloat awf, float* data, int size, struct UpdateOption *options) {
  //  multiverso::UpdateOption mvoptions;
  //  mvoptions.set_learning_rate(options->learning_rate);
  //  mvoptions.set_momentum(options->momentum);
  //  mvoptions.set_rho(options->rho);
  //  ((multiverso::ArrayWorker<float>*) awf)->Add(data, size, &mvoptions);
  //}
}
