#include "multiverso/multiverso.h"

#include "multiverso/zoo.h"

namespace multiverso {

void MultiversoInit(int* argc, char* argv[], int role) {
  Zoo::Get()->Start(argc, argv, role);
}

void MultiversoShutDown(bool finalize_net) {
  Zoo::Get()->Stop(finalize_net);
}

void MultiversoBarrier() {
  Zoo::Get()->Barrier();
}

int MultiversoRank() {
  return Zoo::Get()->rank();
}

}
