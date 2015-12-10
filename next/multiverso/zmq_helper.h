#ifndef MULTIVERSO_ZMQ_HELPER_H_
#define MULTIVERSO_ZMQ_HELPER_H_

#include "zmq_util.h"
#include "arena.h"

namespace multiverso {

void FreeZMQMessage(void* data, void* hint) {
  Arena* arena = static_cast<Arena*>(data);
  bool* last = static_cast<bool*>(hint);
  if (*last) arena->Reset();
}

static bool kLast = true;
static bool kNotLast = false;

zmq::message_t* NewZMQMessage(Arena* arena, size_t size, bool last) {
  char* memory = arena->Allocate(sizeof(zmq::message_t) + size);
  zmq::message_t* msg = new (memory)zmq::message_t(
    memory + sizeof(zmq::message_t), size, FreeZMQMessage, 
    last ? &kLast : &kNotLast);
  return msg;
}

}
#endif