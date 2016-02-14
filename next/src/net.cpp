#include "multiverso/net.h"

#include "multiverso/util/log.h"
#include "multiverso/message.h"

// TODO(feiga) remove this 
#define MULTIVERSO_USE_MPI

namespace multiverso {

#ifdef MULTIVERSO_USE_MPI
#include <mpi.h>

class MPINetWrapper : public NetInterface {
public:
  MPINetWrapper() : more_(std::numeric_limits<int>::max()) {}

  void Init(int* argc, char** argv) override {
    // MPI_Init(argc, &argv);
    MPI_Initialized(&inited_);
    if (!inited_) {
      int flag = 0;
      MPI_Init_thread(argc, &argv, MPI_THREAD_MULTIPLE, &flag);
      CHECK(flag == MPI_THREAD_MULTIPLE);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &size_);
    Log::Info("%s net util inited, rank = %d, size = %d\n", 
      name().c_str(), rank(), size());
  }

  void Finalize() override { MPI_Finalize(); }

  int rank() const override { return rank_; }
  int size() const override { return size_; }
  std::string name() const override { return "MPI"; }

  size_t Send(const MessagePtr& msg) override {
    size_t size = Message::kHeaderSize;
    MPI_Send(msg->header(), Message::kHeaderSize, MPI_BYTE, 
             msg->dst(), 0, MPI_COMM_WORLD);
    // Send multiple msg 
    for (auto& blob : msg->data()) {
      CHECK_NOTNULL(blob.data());
      MPI_Send(blob.data(), static_cast<int>(blob.size()), MPI_BYTE, msg->dst(),
        0, MPI_COMM_WORLD);
      size += blob.size();
    }
    // Send an extra over tag indicating the finish of this Message
    MPI_Send(&more_, sizeof(int), MPI_BYTE, msg->dst(), 
      0, MPI_COMM_WORLD);
    return size + sizeof(int);
  }

  size_t Recv(MessagePtr* msg_ptr) override {
    // Receiving a Message from multiple recv
    MessagePtr& msg = *msg_ptr;
    MPI_Status status;
    MPI_Recv(msg->header(), Message::kHeaderSize, 
             MPI_BYTE, MPI_ANY_SOURCE, 
             0, MPI_COMM_WORLD, &status);
    size_t size = Message::kHeaderSize;
    while (true) { 
      int count;
      MPI_Probe(msg->src(), 0, MPI_COMM_WORLD, &status);
      MPI_Get_count(&status, MPI_BYTE, &count);
      Blob blob(count);
      // We only receive from msg->src() until we recv the overtag msg
      MPI_Recv(blob.data(), count, MPI_BYTE, msg->src(), 
               0, MPI_COMM_WORLD, &status);
      size += count;
      if (count == sizeof(int) && blob.As<int>() == more_) break;
      msg->Push(blob);
    }
    return size;
  }

private:
  const int more_; 
  int inited_;
  int rank_;
  int size_;
};

#endif

// #define MULTIVERSO_USE_ZEROMQ
#ifdef MULTIVERSO_USE_ZEROMQ

#include <zmq.h>

class ZeroMQNetWrapper : public NetInterface {
public:
  void Init(int* argc, char** argv) override {
    context_ = zmq_ctx_new();
    responder_ = zmq_socket(context_, ZMQ_REP);
    CHECK(zmq_bind(responder_, "tcp://*:5555") == 0);
    // get machine file 
    // format is same with MPI machine file

    // TODO(feiga): parse the machine list config file to get the ip
    std::vector<std::string> machine_lists;
    size_ = machine_lists.size();
    for (auto ip : machine_lists) {
      void* requester = zmq_socket(context, ZMQ_REQ);
      zmq_connect(requester, ip.c_str());
      requester_.push_back(requester);
    }

    Log::Info("%s net util inited, rank = %d, size = %d\n",
      name().c_str(), rank(), size());
  }

  void Finalize() override { 
    zmq_close(responder_);
    for (auto& p : requester_) if (p) zmq_close(p);
    zmq_ctx_destroy(context_);
  }

  int rank() const override { return rank_; }
  int size() const override { return size_; }
  std::string name() const override { return "ZeroMQ"; }

  size_t Send(const MessagePtr& msg) override {
    
  }

  size_t Recv(MessagePtr* msg_ptr) override {
    // Receiving a Message from multiple recv
    
  }

private:
  void* context_;
  void* responder_;
  std::vector<void*> requester_;
  const int more_;
  int inited_;
  int rank_;
  int size_;
};

#endif

NetInterface* NetInterface::Get() {
#ifdef MULTIVERSO_USE_ZMQ
  Log::Fatal("Not implemented yet\n");
  net_util = new ZMQNetInterface();
#else 
#ifdef MULTIVERSO_USE_MPI
  static MPINetWrapper net_impl;
#endif
#endif
  return &net_impl; // net_util.get();
}

}
