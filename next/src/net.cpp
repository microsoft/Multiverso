#include "multiverso/net.h"

#include <limits>
#include <mutex>

#include "multiverso/message.h"
// #include "multiverso/net/zmq_net.h"
#include "multiverso/util/log.h"

// TODO(feiga) remove this 
#define MULTIVERSO_USE_MPI

// TODO(feiga): move to seperate files
#ifdef MULTIVERSO_USE_MPI
#include <mpi.h>
#endif


#ifdef _MSC_VER
#undef max
#endif

namespace multiverso {

#ifdef MULTIVERSO_USE_MPI
class MPINetWrapper : public NetInterface {
public:
  MPINetWrapper() : more_(std::numeric_limits<int>::max()) {}

  void Init(int* argc, char** argv) override {
    // MPI_Init(argc, &argv);
    MPI_Initialized(&inited_);
    if (!inited_) {
      MPI_Init_thread(argc, &argv, MPI_THREAD_MULTIPLE, &thread_provided_);
      // CHECK(thread_provided_ == MPI_THREAD_MULTIPLE);
    }
    MPI_Query_thread(&thread_provided_);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &size_);
    Log::Debug("%s net util inited, rank = %d, size = %d\n", 
      name().c_str(), rank(), size());
  }

  void Finalize() override { MPI_Finalize(); }

  int rank() const override { return rank_; }
  int size() const override { return size_; }
  std::string name() const override { return "MPI"; }

  size_t Send(const MessagePtr& msg) override {
    if (thread_provided_ == MPI_THREAD_SERIALIZED) {
      std::lock_guard<std::mutex> lock(mutex_);
      return SendMsg(msg);
    } else if (thread_provided_ == MPI_THREAD_MULTIPLE) {
      return SendMsg(msg);
    } else {
      CHECK(false);
      return 0;
    }
  }

  size_t Recv(MessagePtr* msg) override {
    if (thread_provided_ == MPI_THREAD_SERIALIZED) {
      MPI_Status status;
      int flag;
      // non-blocking probe whether message comes
      MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, &status);
      if (flag) { 
        // a message come
        // block receive with lock guard
        std::lock_guard<std::mutex> lock(mutex_);
        return RecvMsg(msg);
      } else {
        // no message comes
        return 0;
      }
    } else if (thread_provided_ == MPI_THREAD_MULTIPLE) {
      return RecvMsg(msg);
    } else {
      CHECK(false);
      return 0;
    }
  }

  size_t SendMsg(const MessagePtr& msg) {
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
    Log::Debug("MPI-Net: rank %d send msg size = %d\n", rank(), size+4);
    return size + sizeof(int);
  }

  size_t RecvMsg(MessagePtr* msg_ptr) {
    // Receiving a Message from multiple recv
    Log::Debug("MPI-Net: rank %d started recv msg\n", rank());
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
    Log::Debug("MPI-Net: rank %d end recv from src %d, size = %d\n", rank(), msg->src(), size);
    return size;
  }

private:
  const int more_; 
  std::mutex mutex_;
  int thread_provided_;
  int inited_;
  int rank_;
  int size_;
};

#endif



NetInterface* NetInterface::Get() {
#ifdef MULTIVERSO_USE_ZMQ
  Log::Fatal("Not implemented yet\n");
  static ZeroMQNetWrapper net_impl;
#else 
#ifdef MULTIVERSO_USE_MPI
  static MPINetWrapper net_impl;
#endif
#endif
  return &net_impl; // net_util.get();
}

}
