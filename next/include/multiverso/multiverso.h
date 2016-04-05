#ifndef MULTIVERSO_INCLUDE_MULTIVERSO_H_
#define MULTIVERSO_INCLUDE_MULTIVERSO_H_

#include "multiverso/table_interface.h"
#include "multiverso/util/mv_flags.h"
#include <string>


namespace multiverso {

void MV_Init(int* argc = nullptr, 
             char* argv[] = nullptr, 
             int role = 3,
             bool restart = false,
             int store_each_k = 5);

void MV_Barrier(int iter = -1);

void MV_ShutDown(bool finalize_mpi = true);

int  MV_Rank();
int  MV_Size();

int  MV_NumWorkers();
int  MV_NumServers();

int  MV_WorkerId();
int  MV_ServerId();

int  MV_WorkerIdToRank(int worker_id);
int  MV_ServerIdToRank(int server_id);

//new implementation, but template function should be defined in the same file with declaration
/*
 * param table_type the type string of table, such as "matrix","array"
 * param table_args the parameters of table
 * dump_file_path not used now, to be discussed
 */ 
template<typename Key, typename Val=void>
WorkerTable* MV_CreateTable(const std::string& table_type, const std::vector<void*>& table_args, 
  const std::string& dump_file_path = "") {
  return TableFactory::CreateTable<Key, Val>(table_type, table_args, dump_file_path);
}

int MV_LoadTable(const std::string& dump_file_path);

// Show the dashboard information about the monitored excuation time
void MV_Dashboard();

// --- Net API -------------------------------------------------------------- //
// NOTE(feiga): these API is only used for specific situation.
// Init Multiverso Net with the provided endpoint. Multiverso Net will bind 
// the provided endpoint and use this endpoint to listen and recv message
// \param rank the rank of this MV process
// \param endpoint endpoint with format ip:port, e.g., localhost:9999
// \return  0 SUCCESS
// \return -1 FAIL
int  MV_NetBind(int rank, char* endpoint);

// Connect Multiverso Net with other processes in the system. Multiverso Net 
// will connect these endpoints and send msgs
// \param ranks array of rank
// \param endpoints endpoints for each rank
// \param size size of the array
// \return  0 SUCCESS
// \return -1 FAIL
int  MV_NetConnect(int* rank, char* endpoint[], int size);

// define command line flags with bool value
// use -flag=true or -flag=false to use it
// \param name flag name
// \default_value default value for this flag
// \text description
#define MV_DEFINE_bool(name, default_value, text) \
  MV_DEFINE_VARIABLE(bool, name, default_value, text)

// use MV_FLAGS_##name to use this flag
#define MV_DECLARE_bool(name) \
  MV_DECLARE_VARIABLE(bool, name)

// define command line flags with string value
// use MV_FLAGS_##name to use this flag
// \param name flag name
// \default_value default value for this flag
// \text description
#define MV_DEFINE_string(name, default_value, text) \
  MV_DEFINE_VARIABLE(std::string, #name, default_value, text)

#define MV_DECLARE_string(name) \
  MV_DECLARE_VARIABLE(std::string, name)

// define command line flags with int value
// use MV_FLAGS_##name to use this flag
// \param name flag name
// \default_value default value for this flag
// \text description
#define MV_DEFINE_int(name, default_value, text) \
  MV_DEFINE_VARIABLE(int, #name, default_value, text)

#define MV_DECLARE_int(name) \
  MV_DECLARE_VARIABLE(int, name)

// parse registered flags in command line, use -flag=value to change value
// \param argc command line argument number
// \param argc command line variables
// \return unused argument number
int MV_ParseCMDFlags(int argc, char*argv[]);

} // namespace multiverso

#endif // MULTIVERSO_INCLUDE_MULTIVERSO_H_

