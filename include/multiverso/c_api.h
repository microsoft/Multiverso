#ifndef MULTIVERSO_C_API_H_
#define MULTIVERSO_C_API_H_

struct AddOption {
  float learning_rate;
  float momentum;
  float rho;
};

void MV_Init(int* argc, char* argv[]);
void MV_ShutDown();
void MV_Barrier();
typedef void* ArrayWorkerFloat;
typedef void* ArrayServerFloat;
ArrayServerFloat newArrayServerFloat(int);
ArrayWorkerFloat newArrayWorkerFloat(int);
void getArrayWorkerFloat(ArrayWorkerFloat, float*, int);
void addArrayWorkerFloat(ArrayServerFloat, float*, int, struct AddOption*);

#endif
