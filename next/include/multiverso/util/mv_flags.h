#ifndef MULTIVERSO_CMD_FLAGS_H_
#define MULTIVERSO_CMD_FLAGS_H_

#include <string>
#include <unordered_map>

namespace multiverso{

namespace mvflags{

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
    return commands[key].value;
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
struct FlagRegisterHelper{
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
#define MV_DEFINE_VARIABLE(type, name, default_value, text) \
  namespace multiverso{                                     \
    namespace mvflags{                                      \
      multiverso::mvflags::FlagRegisterHelper<type>         \
      g_flag_helper_##name(#name, default_value, text);     \
    }                                                       \
  }

// declare the variable as MV_FLAGS_##name
#define MV_DECLARE_VARIABLE(type, name) \
  type MV_FLAGS_##name = multiverso::mvflags::g_flag_helper_##name.command->value;

// Parse command line flags and return unused argument number
int ParseCMDFlags(int* argc, char*argv[]);

}//namespace mv flags
}//namespace multiverso
#endif