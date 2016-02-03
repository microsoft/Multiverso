#include "multiverso/multiverso.h"

#include "zoo.h"

namespace multiverso {

void MultiversoInit(int role) {
  Zoo::Get()->Start(role);
}

void MultiversoShutDown(bool finalize_net) {
  Zoo::Get()->Stop(finalize_net);
}

void MultiversoBarrier() {
  Zoo::Get()->Barrier();
}

}
