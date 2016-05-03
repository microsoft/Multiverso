#include "multiverso/table_factory.h"

#include "multiverso/table/array_table.h"
#include "multiverso/table/matrix_table.h"

namespace multiverso {

std::unordered_map<std::string, 
  std::pair<worker_table_creater_t, server_table_creater_t> > 
  TableFactory::table_creaters_;

std::string ele_type_str(EleType ele_type) {
  switch (ele_type) {
  case kInt:
    return "int_";
  case kFloat:
    return "float_";
  case kDouble:
    return "kdouble_";
  }
  return "unknown";
}

WorkerTable* TableFactory::CreateTable(
  std::string& type,
  void**table_args) {
  CHECK(table_creaters_.find(type) != table_creaters_.end());

  if (MV_ServerId() >= 0) {
    table_creaters_[type].second(table_args);
  }
  if (MV_WorkerId() >= 0) {
    return table_creaters_[type].first(table_args);
  }
  return nullptr;
}

WorkerTable* TableFactory::CreateTable(
  EleType ele_type,
  std::string& type,
  void**table_args) {
  std::string typestr = ele_type_str(ele_type) + type;  
  return CreateTable(typestr, table_args);
}

WorkerTable* TableFactory::CreateTable(
  EleType ele_type1,
  EleType ele_type2,
  std::string& type,
  void**table_args) {
  std::string typestr = ele_type_str(ele_type1) 
    + ele_type_str(ele_type2) + type;
  return CreateTable(typestr, table_args);
}

void TableFactory::RegisterTable(
  std::string& type,
  worker_table_creater_t wt,
  server_table_creater_t st) {
  CHECK(table_creaters_.find(type) == table_creaters_.end());

  table_creaters_[type] = std::make_pair(wt, st);
}

#define MV_REGISTER_TABLE(type, worker_table_creater,  \
  server_table_creater)                                \
  namespace table_factory {                            \
   TableRegister type##_table_register(#type,          \
      worker_table_creater,                            \
      server_table_creater);                           \
  }

#define MV_REGISTER_TABLE_WITH_BASIC_TYPE(table_name,                 \
  worker_table_creater, server_table_creater)                         \
  MV_REGISTER_TABLE(int_##table_name, worker_table_creater<int>,      \
    server_table_creater<int>);                                       \
  MV_REGISTER_TABLE(float_##table_name, worker_table_creater<float>,  \
    server_table_creater<float>);                                     \
  MV_REGISTER_TABLE(double_##table_name, worker_table_creater<double>,\
    server_table_creater<double>);                                 

template<typename T>
WorkerTable* create_array_worker(void **args) {
  return new ArrayWorker<T>(*(size_t*)(*args));
}
template<typename T>
ServerTable* create_array_server(void **args) {
  return new ArrayServer<T>(*(size_t*)(*args));
}
MV_REGISTER_TABLE_WITH_BASIC_TYPE(array, create_array_worker, create_array_server);

template<typename T>
WorkerTable* create_matrix_worker(void **args) {
  return new MatrixWorkerTable<T>(*(integer_t*)(*args), *(integer_t*)(*(args+1)));
}
template<typename T>
ServerTable* create_matrix_server(void **args) {
  return new MatrixServerTable<T>(*(integer_t*)(*args), *(integer_t*)(*(args + 1)));
}
MV_REGISTER_TABLE_WITH_BASIC_TYPE(matrix, create_matrix_worker, create_matrix_server);

} // namespace multiverso