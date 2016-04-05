#include "multiverso/util/mv_flags.h"

#include <algorithm>


namespace multiverso{
namespace mvflags{

int ParseCMDFlags(int* argc, char*argv[]){
  int unused = 1;

  int intval;
  bool boolval;
  std::string text;
  for (int i = 1; i < *argc; ++i){
    text = argv[i];
    if (text.find("-") != 0) {
      continue;
    }

    size_t split = text.find("=");
    std::string& key = text.substr(1, split - 1);
    std::string& val = text.substr(split + 1);

    if (FlagRegister<std::string>::Get()->SetFlagIfFound(key, val))
      continue;

    intval = atoi(val.c_str());
    if (FlagRegister<int>::Get()->SetFlagIfFound(key, intval))
      continue;

    transform(val.begin(), val.end(), val.begin(), ::tolower); //transform to lower case
    boolval = (val == "true");
    if (FlagRegister<bool>::Get()->SetFlagIfFound(key, boolval))
      continue;

    std::swap(argv[unused++], argv[i]);
  }
  *argc = unused;
  return unused;
}

}
}