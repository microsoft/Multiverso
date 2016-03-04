#ifndef MULTIVERSO_INCLUDE_MULTIVERSO_H_
#define MULTIVERSO_INCLUDE_MULTIVERSO_H_

namespace multiverso {

enum Role {
  Null = 0,
  Worker = 1,
  Server = 2,
  All = 3
};

void MV_Init(int* argc = nullptr, 
             char* argv[] = nullptr, 
             int role = All);

void MV_Barrier();

void MV_ShutDown(bool finalize_mpi = true);

int  MV_Rank();
int  MV_Size();

int  MV_Num_Workers();
int  MV_Num_Servers();

int  MV_Worker_Id();
int  MV_Server_Id();

// --- Net API -------------------------------------------------------------- //
// NOTE(feiga): these API is only used for specific situation.
// Init Multiverso Net with the provided endpoint. Multiverso Net will bind 
// the provided endpoint and use this endpoint to listen and recv message
// \param rank the rank of this MV process
// \param endpoint endpoint with format ip:port, e.g., localhost:9999
// \return  0 SUCCESS
// \return -1 FAIL
int  MV_Net_Bind(int rank, char* endpoint);

// Connect Multiverso Net with other processes in the system. Multiverso Net 
// will connect these endpoints and send msgs
// \param ranks array of rank
// \param endpoints endpoints for each rank
// \param size size of the array
// \return  0 SUCCESS
// \return -1 FAIL
int  MV_Net_Connect(int* rank, char* endpoint[], int size);

} // namespace multiverso

#endif // MULTIVERSO_INCLUDE_MULTIVERSO_H_