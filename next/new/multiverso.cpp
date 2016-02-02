#include "multiverso.h"

#include "zoo.h"

namespace multiverso {

void MultiversoInit() {
  Zoo::Get()->Start();
}

void MultiversoShutDown() {
  Zoo::Get()->Stop();
}

void MultiversoBarrier() {
  Zoo::Get()->Barrier();
}

}
