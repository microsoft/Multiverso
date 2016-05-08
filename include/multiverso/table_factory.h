#ifndef MULTIVERSO_TABLE_FACTORY_H_
#define MULTIVERSO_TABLE_FACTORY_H_

#include "multiverso/table_interface.h"
#include "multiverso.h"

#include <string>

namespace multiverso {

typedef WorkerTable* (*worker_table_creater_t)(void*table_args);
typedef ServerTable* (*server_table_creater_t)(void*table_args);

class TableFactory {
public:
  template <typename EleType, typename OptionType>
  static typename trait::OptionTrait<EleType, OptionType>::worker_table_type* 
    CreateTable(const OptionType& option) {
    std::string typestr = typeid(EleType).name() + std::string("_") +
      trait::OptionTrait<EleType, OptionType>::type;
    return InnerCreateTable<EleType, OptionType>(typestr, option);
  }
  static void RegisterTable(
    std::string& type,
    worker_table_creater_t wt,
    server_table_creater_t st);
  static void FreeServerTables();
private:
  template <typename EleType, typename OptionType>
  static typename trait::OptionTrait<EleType, OptionType>::worker_table_type*
    InnerCreateTable(const std::string& type,
    const OptionType& table_args) {
    CHECK(table_creaters_.find(type) != table_creaters_.end());

    if (MV_ServerId() >= 0) {
      table_factory::PushServerTable(table_creaters_[type].second((void*)&table_args));
    }
    if (MV_WorkerId() >= 0) {
      return reinterpret_cast<trait::OptionTrait<EleType, OptionType>::worker_table_type*>
        (table_creaters_[type].first((void*)&table_args));
    }
    return nullptr;
  }
  TableFactory() = default;
  static std::unordered_map<
    std::string,
    std::pair<worker_table_creater_t,
    server_table_creater_t> > table_creaters_;
};

namespace table_factory {
struct TableRegister {
  TableRegister(std::string type, worker_table_creater_t wt,
    server_table_creater_t st) {
    TableFactory::RegisterTable(type, wt, st);
  }
};
void FreeServerTables();
void PushServerTable(ServerTable*table);
} // namespace table_factory

} // namespace multiverso

#endif // MULTIVERSO_TABLE_FACTORY_H_