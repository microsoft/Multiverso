#include "multiverso/server.h"

#include "multiverso/actor.h"
#include "multiverso/table_interface.h" 
#include "multiverso/zoo.h"
#include "multiverso/util/io.h"

#include "multiverso/dashboard.h"

namespace multiverso {

Server::Server() : Actor(actor::kServer) {
  RegisterHandler(MsgType::Request_Get, std::bind(
    &Server::ProcessGet, this, std::placeholders::_1));
  RegisterHandler(MsgType::Request_Add, std::bind(
    &Server::ProcessAdd, this, std::placeholders::_1));
}

int Server::RegisterTable(ServerTable* server_table) {
  int id = static_cast<int>(store_.size());
  store_.push_back(server_table);
  return id;
}

void Server::ProcessGet(MessagePtr& msg) {
  MONITOR_BEGIN(SERVER_PROCESS_GET);
  MessagePtr reply(msg->CreateReplyMessage());
  int table_id = msg->table_id();
  CHECK(table_id >= 0 && table_id < store_.size());
  store_[table_id]->ProcessGet(msg->data(), &reply->data());
  SendTo(actor::kCommunicator, reply);
  MONITOR_END(SERVER_PROCESS_GET);
}

void Server::ProcessAdd(MessagePtr& msg) {
  MONITOR_BEGIN(SERVER_PROCESS_ADD)
  MessagePtr reply(msg->CreateReplyMessage());
  int table_id = msg->table_id();
  CHECK(table_id >= 0 && table_id < store_.size());
  store_[table_id]->ProcessAdd(msg->data());
  SendTo(actor::kCommunicator, reply);
  MONITOR_END(SERVER_PROCESS_ADD)
}

void Server::SetTableFilePath(const std::string& table_file_path){
  int id = Zoo::Get()->server_rank();
  std::string  server_id_str = (id == 0 ? "0" : "");
  while (id > 0){
    server_id_str = (char)((id % 10) + '0') + server_id_str;
    id /= 10;
  }
  table_file_path_ = table_file_path + server_id_str;
}

void Server::StoreTable(int epoch){
  Stream* stream = StreamFactory::GetStream(URI(table_file_path_), FileOpenMode::Write);
  stream->Write(&epoch, sizeof(int));
  for (int i = 0; i < store_.size(); ++i){
    store_[i]->Store(stream);
  }
  delete stream;
}

int Server::LoadTable(const std::string& file_path){
  Stream* stream = StreamFactory::GetStream(URI(table_file_path_), FileOpenMode::Read);
  if (!stream->Good()) {
    Log::Error("Rank %d open file %s error in Server::LoadTable\n", Zoo::Get()->rank(), file_path.c_str());
    delete stream;
    return 0; //open file error, may not exist
  }

  int iter;
  size_t readsize = stream->Read(&iter, sizeof(int));
  if (readsize == 0) {
    Log::Error("Rank %d read file %s no data in Server::LoadTable\n", Zoo::Get()->rank(), file_path.c_str());
    delete stream;
    return 0; //no store data
  }

  for (int i = 0; i < store_.size(); ++i){
    store_[i]->Load(stream);
  }

  delete stream;
  return iter + 1; //the next iteration number
}
}