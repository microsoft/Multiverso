#include "multiverso/server.h"

#include <string>

#include "multiverso/actor.h"
#include "multiverso/dashboard.h"
#include "multiverso/table_interface.h"
#include "multiverso/io/io.h"
#include "multiverso/zoo.h"


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

}  // namespace multiverso
