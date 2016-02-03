#include "multiverso/multiverso.h"

#include "zoo.h"

namespace multiverso {

void MultiversoInit(int role) {
  Zoo::Get()->Start(role);
}

void MultiversoShutDown() {
  Zoo::Get()->Stop();
}

void MultiversoBarrier() {
  Zoo::Get()->Barrier();
}

}
