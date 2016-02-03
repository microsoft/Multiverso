#include "controller.h"

#include "message.h"
#include "zoo.h"

namespace multiverso {

class Controller::BarrierController {
public:
  explicit BarrierController(Controller* parent) : parent_(parent) {}

  void Control(MessagePtr& msg) {
    tasks_.push_back(std::move(msg));
    if (tasks_.size() == Zoo::Get()->size()) {
      MessagePtr my_reply; // my reply should be the last one
      for (auto& msg : tasks_) {
        MessagePtr reply(msg->CreateReplyMessage());
        if (reply->dst() != Zoo::Get()->rank()) {
          parent_->DeliverTo(actor::kCommunicator, reply);
        } else {
          my_reply = std::move(reply);
        }
      }
      parent_->DeliverTo(actor::kCommunicator, my_reply);
      tasks_.clear();
    }
  }
private:
  std::vector<MessagePtr> tasks_;
  Controller* parent_; // not owned
}; 

Controller::Controller() : Actor(actor::kController) {
  RegisterTask(MsgType::Control_Barrier, std::bind(
    &Controller::ProcessBarrier, this, std::placeholders::_1));
  barrier_controller_ = new BarrierController(this);
}

void Controller::ProcessBarrier(MessagePtr& msg) {
  barrier_controller_->Control(msg);
}

}