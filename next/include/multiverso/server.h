#ifndef MULTIVERSO_SERVER_H_
#define MULTIVERSO_SERVER_H_

#include "multiverso/actor.h"
#include <vector>

namespace multiverso {

class ServerTable;

class Server : public Actor { 
public:
  Server();
  int RegisterTable(ServerTable* table);
  //store server data to file
  void StoreTable(int epoch);
  //load data from file and return next iteration number
  int LoadTable(const std::string& file_path);
  void SetDumpFilePath(const std::string& dump_file_path);
private:
  void ProcessGet(MessagePtr& msg);
  void ProcessAdd(MessagePtr& msg);
  std::string dump_file_path_;
  // contains the parameter data structure and related handle method
  std::vector<ServerTable*> store_;
};

}
#endif // MULTIVERSO_SERVER_H_