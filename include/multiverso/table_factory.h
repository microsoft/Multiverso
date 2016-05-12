#ifndef MULTIVERSO_TABLE_FACTORY_H_
#define MULTIVERSO_TABLE_FACTORY_H_

#include "table_interface.h"
#include "zoo.h"

#include <string>

namespace multiverso {

namespace table_factory {

template <typename OptionType>
typename OptionType::WorkerTableType* CreateTable(const OptionType& option) {
  if (Zoo::Get()->server_rank() >= 0) {
    table_factory::PushServerTable(
      new OptionType::ServerTableType(option));
  }
  if (Zoo::Get()->worker_rank() >= 0) {
    return new OptionType::WorkerTableType(option);
  }
  return nullptr;
}

void FreeServerTables();
void PushServerTable(ServerTable*table);

} // namespace table_factory

} // namespace multiverso

#endif // MULTIVERSO_TABLE_FACTORY_H_