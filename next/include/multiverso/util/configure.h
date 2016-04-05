#ifndef MULTIVERSO_CMD_FLAGS_H_
#define MULTIVERSO_CMD_FLAGS_H_

#include <string>
#include <unordered_map>
#include <functional>

namespace multiverso{

class Configure {
public:
  using Handler = std::function<void(const std::string&, const std::string&)>;

  static void ConfigureFile(const std::string &config_file);
  static int ParseCMDFlags(int*argc, char*argv[]);
  static void RegisterHandler(const std::string& key, const Handler & task) {
    configure_.handlers_.insert({ key, task });
  }

private:
  Configure();
  Configure(Configure&) = delete;
  bool ProcessLine(const std::string& line, int start_pos = 0);

  static void ProcessLogLevel(const std::string &key, const std::string &value);
  static void ProcessRole(const std::string &key, const std::string &value);
private:
  static Configure configure_;
  std::unordered_map<std::string, Handler> handlers_;
};

namespace configures{

template<typename T>
struct Command {
  T value;
  std::string description;
};

//used to register and keep flags
template<typename T>
class FlagRegister {
public:
  Command<T>* RegisterFlag(const std::string& name, T& default_value, const std::string& text){
    commands[name] = { default_value, text };
    return &commands[name];
  }

  //set flag value if in the defined list
  bool SetFlagIfFound(const std::string& key, const T& value){
    if (commands.find(key) != commands.end()) {
      commands[key].value = value;
      return true;
    }
    return false;
  }

  T& GetValue(const std::string& name){
    return commands[name].value;
  }

  //get flag register instance
  static FlagRegister* Get() {
    static FlagRegister register_;
    return &register_;
  }
private:
  std::unordered_map<std::string, Command<T>> commands;
private:
  FlagRegister(){}
  FlagRegister(FlagRegister<T>&) = delete;
};

template<typename T>
class FlagRegisterHelper{
public:
  FlagRegisterHelper(const std::string name, T val, const std::string &text){
    command = FlagRegister<T>::Get()->RegisterFlag(name, val, text);
  }
  Command<T> *command;
};

// register a flag
// \param type variable type
// \param name variable name
// \param default_vale
// \text description
#define DEFINE_CONFIGURE(type, name, default_value, text)                        \
  namespace configures {                                                            \
    FlagRegisterHelper<type> g_configure_helper_##name(#name, default_value, text); \
  }

// declare the variable as MV_FLAGS_##name
#define DECLARE_CONFIGURE(type, name) \
  const type& MV_CONFIG_##name = configures::g_configure_helper_##name.command->value;

}//namespace configures

}//namespace multiverso

#endif