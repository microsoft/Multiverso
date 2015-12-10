#include "arena.h"

#include <iostream>
#include <zmq.hpp>
#include <stop_watch.h>
#include <zmq_util.h>

int main()
{
  multiverso::Arena arena;

  std::cout << arena.DebugString();
  
  //zmq::message_t* msg = new (arena.Allocate(sizeof(zmq::message_t)))
  //    zmq::message_t(3 * sizeof(int));
  multiverso::StopWatch watch;
  for (int i = 0; i < 10000; ++i){
    zmq::message_t* msg = multiverso::ZMQUtil::NewZMQMessage(&arena, 256, i == 9);
    //std::cout << "msg size = " << msg->size() << std::endl;

    //std::cout << arena.DebugString();
  }
  std::cout << "arena time : " << watch.ElapsedSeconds() << std::endl;
  std::cout << arena.DebugString();

  arena.Reset();

  watch.Restart();
  for (int i = 0; i < 10000; ++i){
    zmq::message_t* msg = multiverso::ZMQUtil::NewZMQMessage(&arena, 256, i == 9);
    //std::cout << "msg size = " << msg->size() << std::endl;

    //std::cout << arena.DebugString();
  }
  std::cout << "arena time : " << watch.ElapsedSeconds() << std::endl;
  std::cout << arena.DebugString();

  watch.Restart();
  for (int i = 0; i < 10000; ++i)
  {
      zmq::message_t* msg = new zmq::message_t(256);
  }
  std::cout << "naive time : " << watch.ElapsedSeconds() << std::endl;
  return 0;
}
