#include "multiverso/util/configure.h"
#include "multiverso/util/io.h"
#include "multiverso/util/log.h"

#include <algorithm>

namespace multiverso{

Configure Configure::configure_;

Configure::Configure(){
  RegisterHandler("log_level", std::bind(
    &Configure::ProcessLogLevel, std::placeholders::_1, std::placeholders::_2));
  RegisterHandler("role", std::bind(
    &Configure::ProcessRole, std::placeholders::_1, std::placeholders::_2));
}

void Configure::ConfigureFile(const std::string &config_file) {
  if (config_file == "")
    return;

  TextReader text_reader(URI(config_file), 1024);
  std::string line;
  while ((text_reader.GetLine(line)) > 0){
    configure_.ProcessLine(line);
  }
}

int Configure::ParseCMDFlags(int*argc, char*argv[]){
  int unused = 1;
  
  std::string text;
  for (int i = 1; i < *argc; ++i){
    text = argv[i];
    if (text.find("-") != 0) {
      continue;
    }
    if (configure_.ProcessLine(text, 1))
      continue;
    std::swap(argv[unused++], argv[i]);
  }

  *argc = unused;

  return unused;
}

bool Configure::ProcessLine(const std::string &line, int start_pos){
  using namespace configures;

  size_t pos = line.find("=");
  CHECK(pos != -1);

  std::string key = line.substr(start_pos, pos - start_pos);
  std::string value = line.substr(pos + 1);

  if (handlers_.find(key) != handlers_.end()) {
    handlers_[key](key, value);
    return true;
  }
  if (FlagRegister<std::string>::Get()->SetFlagIfFound(key, value))
    return true;

  int intval = atoi(value.c_str());
  if (FlagRegister<int>::Get()->SetFlagIfFound(key, intval))
    return true;

  transform(value.begin(), value.end(), value.begin(), ::tolower); //transform to lower case
  bool boolval = (value == "true");
  if (FlagRegister<bool>::Get()->SetFlagIfFound(key, boolval))
    return true;

  return false;
}

void Configure::ProcessLogLevel(const std::string& key, const std::string &value) {
  LogLevel val;
  if (value == "Debug") val = LogLevel::Debug;
  else if (value == "Info") val = LogLevel::Info;
  else if (value == "Fatal") val = LogLevel::Fatal;
  else if (value == "Error") val = LogLevel::Error;
  else CHECK(false);

  configures::FlagRegister<LogLevel>::Get()->SetFlagIfFound(key, val);
}

void Configure::ProcessRole(const std::string& key, const std::string &value) {
  if (value == "All")  return;
  int role = 3;
  if (value == "Null") role = 0;
  else if (value == "Worker") role = 1;
  else if (value == "Server") role = 2;
  else CHECK(false);

  configures::FlagRegister<int>::Get()->SetFlagIfFound(key, role);
}

}