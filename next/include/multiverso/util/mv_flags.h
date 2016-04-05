#ifndef MULTIVERSO_CMD_FLAGS_H_
#define MULTIVERSO_CMD_FLAGS_H_

#include <string>
#include <unordered_map>

namespace multiverso{

namespace mvflags{

//used to register and keep flags
template<typename T>
class FlagRegister {
public:
  struct Command {
    T* value;
    std::string description;
  };

  void RegisterFlag(const std::string& name, T* default_value, const std::string& text){
    commands[name] = { default_value, text };
  }

  //set flag value if in the defined list
  bool SetFlagIfFound(const std::string& key, const T& value){
    if (commands.find(key) != commands.end()) {
      *commands[key].value = value;
      return true;
    }
    return false;
  }

  //get flag register instance
  static FlagRegister* Get() {
    static FlagRegister register_;
    return &register_;
  }
private:
  std::unordered_map<std::string, Command> commands;
private:
  FlagRegister(){}
  FlagRegister(FlagRegister<T>&) = delete;
};

template<typename T>
struct FlagRegisterHelper{
  FlagRegisterHelper(const std::string name, T*val, const std::string &text){
    FlagRegister<T>::Get()->RegisterFlag(name, val, text);
  }
};

// register a global variable MV_FLAGS_##name
// \param type variable type
// \param name variable name
// \param default_vale
// \text description
#define MV_DEFINE_VARIABLE(type, name, default_value, text) \
  type MV_FLAGS_##name = default_value;                     \
  namespace mvflags{                                       \
  multiverso::mvflags::FlagRegisterHelper<type> g_cmd_flag_helper_##name(#name, &MV_FLAGS_##name, text); \
  }

// Parse command line flags and return unused argument number
int ParseCMDFlags(int* argc, char*argv[]);

}//namespace cmd flags
}//namespace multiverso
#endif