#include <multiverso/table/array_table.h>
#include <multiverso/util/log.h>
#include <multiverso/multiverso.h>
#include <multiverso/updater/updater.h>

extern "C" {
  #include <multiverso/c_api.h>
}


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

  ArrayServerFloat newArrayServerFloat(int i) {
    return (ArrayServerFloat) new multiverso::ArrayServer<float>(i);
  }

  ArrayWorkerFloat newArrayWorkerFloat(int i) {
    return (ArrayWorkerFloat) new multiverso::ArrayWorker<float>(i);
  }

  void getArrayWorkerFloat(ArrayWorkerFloat awf, float* data, int size) {
    ((multiverso::ArrayWorker<float>*) awf)->Get(data, size);
  }

  void addArrayWorkerFloat(ArrayServerFloat awf, float* data, int size, struct AddOption *options) {
    multiverso::AddOption mvoptions;
    mvoptions.set_learning_rate(options->learning_rate);
    mvoptions.set_momentum(options->momentum);
    mvoptions.set_rho(options->rho);
    ((multiverso::ArrayWorker<float>*) awf)->Add(data, size, &mvoptions);
  }
}
