#ifndef MULTIVERSO_C_API_H_
#define MULTIVERSO_C_API_H_

#if defined _WIN32
#define DllExport __declspec(dllexport)
#else
#define DllExport
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void* TableHandle;

DllExport void MV_Init(int* argc, char* argv[]);

DllExport void MV_ShutDown();

DllExport void MV_Barrier();

DllExport void MV_NewTable(int size, TableHandle* out);

DllExport void MV_Get(TableHandle handle, float* data, int size);

DllExport void MV_Add(TableHandle handle, float* data, int size);

// typedef void* ArrayWorkerFloat;
// typedef void* ArrayServerFloat;
// struct UpdateOption {
//  float learning_rate;
//  float momentum;
//  float rho;
// };
// ArrayServerFloat newArrayServerFloat(int);
// ArrayWorkerFloat newArrayWorkerFloat(int);
// void getArrayWorkerFloat(ArrayWorkerFloat, float*, int);
// void addArrayWorkerFloat(ArrayServerFloat, float*, int, struct UpdateOption*);

#ifdef __cplusplus
}  // end extern "C"
#endif

#endif  // MULTIVERSO_C_API_H_
