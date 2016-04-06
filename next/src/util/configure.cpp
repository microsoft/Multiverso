#include "multiverso/util/configure.h"
#include "multiverso/util/log.h"

#include <algorithm>

namespace multiverso{

void ParseCMDFlags(int*argc, char*argv[]){
  using namespace configures;
  int unused = 1;
  
  size_t pos;
  int intval;
  bool boolval;
  std::string line, key, value;

  for (int i = 1; i < *argc; ++i){
    line = argv[i];
    if (line.find("-") != 0) {
      continue;
    }

    pos = line.find("=");
    CHECK(pos != -1);

    key = line.substr(1, pos - 1);
    value = line.substr(pos + 1);

    if (FlagRegister<std::string>::Get()->SetFlagIfFound(key, value))
      continue;

    intval = atoi(value.c_str());
    if (FlagRegister<int>::Get()->SetFlagIfFound(key, intval))
      continue;

    transform(value.begin(), value.end(), value.begin(), ::tolower); //transform to lower case
    boolval = (value == "true");
    if (FlagRegister<bool>::Get()->SetFlagIfFound(key, boolval))
      continue;

    std::swap(argv[unused++], argv[i]);
  }

  *argc = unused;
}

}