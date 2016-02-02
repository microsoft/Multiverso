#ifndef MULTIVERSO_CONTROLLER_H_
#define MULTIVERSO_CONTROLLER_H_

#include "actor.h"
#include "message.h"

namespace multiverso {

class Controller : public Actor {
public:
  Controller();
private:
  void ProcessBarrier(MessagePtr& msg);

  // TODO(feiga): may delete the following
  class BarrierController;
  BarrierController* barrier_controller_;
  class ClockController;
  ClockController* clock_controller_;
};

} // namespace multiverso

#endif // MULTIVERSO_CONTROLLER_H_