#include "multiverso/table_factory.h"

#include "multiverso/table/array_table.h"
#include "multiverso/table/matrix_table.h"

namespace multiverso {

std::unordered_map<std::string, 
  std::pair<worker_table_creater_t, server_table_creater_t> > 
  TableFactory::table_creaters_;

void TableFactory::RegisterTable(
  std::string& type,
  worker_table_creater_t wt,
  server_table_creater_t st) {
  CHECK(table_creaters_.find(type) == table_creaters_.end());

  table_creaters_[type] = std::make_pair(wt, st);
}

namespace table_factory {
std::vector<ServerTable*> g_server_tables;
void FreeServerTables() {
  for (auto table : g_server_tables) {
    delete table;
  }
}
void PushServerTable(ServerTable*table) {
  g_server_tables.push_back(table);
}
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
WorkerTable* create_array_worker(void *args) {
  return new ArrayWorker<T>(((ArrayTableInitOption*)args)->size);
}
template<typename T>
ServerTable* create_array_server(void *args) {
  return new ArrayServer<T>(((ArrayTableInitOption*)args)->size);
}
MV_REGISTER_TABLE_WITH_BASIC_TYPE(array, create_array_worker, create_array_server);

namespace trait {
#define DECLARE_OPTION_TRAIT_TYPE(eletype, optiontype, str)  \
  template<>                                                 \
  std::string OptionTrait<eletype, optiontype>::type = #str;

#define DECLARE_OPTION_TRAIT_WITH_BASIC_TYPE(optiontype, str) \
  DECLARE_OPTION_TRAIT_TYPE(int, optiontype, str)             \
  DECLARE_OPTION_TRAIT_TYPE(float, optiontype, str)           \
  DECLARE_OPTION_TRAIT_TYPE(double, optiontype, str)          \

DECLARE_OPTION_TRAIT_WITH_BASIC_TYPE(ArrayTableInitOption, array);
}

} // namespace multiverso