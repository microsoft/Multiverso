#include "multiverso/util/configure.h"

#include <algorithm>
#include <string>
#include "multiverso/util/log.h"

namespace multiverso {

void ParseCMDFlags(int* argc, char* argv[]) {
  if (argc == nullptr || argv == nullptr) return;

  int unused = 1;
  size_t pos;
  int intval;
  bool boolval;
  std::string line, key, value;

  for (int i = 1; i < *argc; ++i) {
    line = argv[i];
    if (line.find("-") != 0) {
      continue;
    }

    pos = line.find("=");
    CHECK(pos != -1);

    key = line.substr(1, pos - 1);
    value = line.substr(pos + 1);

    if (configure::FlagRegister<std::string>::Get()->SetFlagIfFound(key, value))
      continue;

    intval = atoi(value.c_str());
    if (configure::FlagRegister<int>::Get()->SetFlagIfFound(key, intval))
      continue;

    transform(value.begin(), value.end(), value.begin(), ::tolower);
    boolval = (value == "true");
    if (configure::FlagRegister<bool>::Get()->SetFlagIfFound(key, boolval))
      continue;

    std::swap(argv[unused++], argv[i]);
  }

  *argc = unused;
}

}  // namespace multiverso
