#ifndef MULTIVERSO_CONFIGURE_H_
#define MULTIVERSO_CONFIGURE_H_

/*
 * brief Assumed to load the configuration
 */

#include "multiverso/util/io.h"

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace multiverso {

#ifndef MULTIVERSO_INCLUDE_MULTIVERSO_H_
enum Role {
  Null = 0,
  Worker = 1,
  Server = 2,
  All = 3
};
#endif

class Configure {
public:
  using Handler = std::function<void(const std::string&)>;

  Configure(const char* configure_file): restart_(false) {
    RegisterHandler("LogLevel", std::bind(
      &Configure::ProcessLogLevel, this, std::placeholders::_1));
    RegisterHandler("NumNode", std::bind(
      &Configure::ProcessNodeNumber, this, std::placeholders::_1));
    RegisterHandler("Role", std::bind(
      &Configure::ProcessRole, this, std::placeholders::_1));
    RegisterHandler("Restart", std::bind(
      &Configure::ProcessRestart, this, std::placeholders::_1));

    //a function as an interface for user to register their own configuration

    TextReader text_reader(URI(configure_file), 1024);
    std::string line, key, value;
    int pos = -1;
    while ((text_reader.GetLine(line)) > 0){
      pos = line.find("=");
      CHECK(pos != -1);
      key = line.substr(0, pos);
      value = line.substr(pos, line.length - pos);
      if (handlers_.find(key) != handlers_.end()) {
        handlers_[key](value);
      }
      else configure_[key] = value;
    }

    CHECK(roles_.size() > 0);
  }

  void RegisterHandler(const std::string& key, const Handler & task) {
    handlers_.insert({ key, task });
  }

  std::string configure(const std::string& key) {return configure_[key];}
  LogLevel log_level() const { return log_level_;  }
  Role role(const int rank) const { return roles_[rank]; }
private:
  void ProcessRestart(const std::string &value) {
    if (value == "TRUE") restart_ = true;
    else if (value == "FALSE") restart_ = false;
    else CHECK(false);
  }

  void ProcessLogLevel(const std::string &value) {
    if (value == "Debug") log_level_ = LogLevel::Debug;
    else if (value == "Info") log_level_ = LogLevel::Info;
    else if (value == "Fatal") log_level_ = LogLevel::Fatal;
    else if (value == "Error") log_level_ = LogLevel::Error;
    else CHECK(false);
  }

  void ProcessRole(const std::string &value) {
    int pos = value.find(' ');
    CHECK(pos >= 0);
    int rank = atoi(value.substr(0, pos).c_str());
    CHECK(rank >= 0);
    std::string role = value.substr(pos, value.length - pos);
    if (role == "All")  return;
    if (role == "Null") roles_[rank] = Role::Null;
    else if (role == "Server") roles_[rank] = Role::Server;
    else if (role == "Worker") roles_[rank] = Role::Worker;
    else CHECK(false);
  }

  void ProcessNodeNumber(const std::string&value) {
    int num = atoi(value.c_str());
    roles_ = std::vector<Role>(num, Role::All);
  }

  bool restart_;
  LogLevel log_level_;
  std::vector<Role> roles_;
  std::map<std::string, Handler> handlers_;
  std::map<std::string, std::string> configure_;
};

}
#endif