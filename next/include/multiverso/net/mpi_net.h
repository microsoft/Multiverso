#ifndef MULTIVERSO_NET_MPI_NET_H_
#define MULTIVERSO_NET_MPI_NET_H_

#ifdef MULTIVERSO_USE_MPI

#include "multiverso/net.h"

#include <limits>
#include <mutex>

#include "multiverso/message.h"
#include "multiverso/util/log.h"

#include <mpi.h>


#ifdef _MSC_VER
#undef max
#endif

namespace multiverso {

class MPINetWrapper : public NetInterface {
public:
  MPINetWrapper() : more_(std::numeric_limits<char>::max()) {}

  void Init(int* argc, char** argv) override {
    // MPI_Init(argc, &argv);
    MPI_Initialized(&inited_);
    if (!inited_) {
      MPI_Init_thread(argc, &argv, MPI_THREAD_MULTIPLE, &thread_provided_);
    }
    MPI_Query_thread(&thread_provided_);
    if (thread_provided_ < MPI_THREAD_SERIALIZED) {
      Log::Fatal("At least MPI_THREAD_SERIALIZED supported is needed by multiverso.\n");
    }
    else if (thread_provided_ == MPI_THREAD_SERIALIZED) {
      Log::Info("multiverso MPI-Net is initialized under MPI_THREAD_SERIALIZED mode.\n");
    }
    else if (thread_provided_ == MPI_THREAD_MULTIPLE) {
      Log::Debug("multiverso MPI-Net is initialized under MPI_THREAD_MULTIPLE mode.\n");
    }
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
      //std::lock_guard<std::mutex> lock(mutex_);
      return SendMsg(msg);
    }
    else if (thread_provided_ == MPI_THREAD_MULTIPLE) {
      return SendMsg(msg);
    }
    else {
      CHECK(false);
      return 0;
    }
  }

  size_t Recv(MessagePtr* msg) override {
    MPI_Status status;
    int flag;
    // non-blocking probe whether message comes
    MPI_Iprobe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &flag, &status);
    if (!flag) return 0;
    if (thread_provided_ == MPI_THREAD_SERIALIZED) {
      // a message come
      // block receive with lock guard
      //std::lock_guard<std::mutex> lock(mutex_);
      return RecvMsg(msg);
    }
    else if (thread_provided_ == MPI_THREAD_MULTIPLE) {
      return RecvMsg(msg);
    }
    else {
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
    MPI_Send(&more_, sizeof(char), MPI_BYTE, msg->dst(),
      0, MPI_COMM_WORLD);
    // Log::Debug("MPI-Net: rank %d send msg size = %d\n", rank(), size+4);
    return size + sizeof(char);
  }

  size_t RecvMsg(MessagePtr* msg_ptr) {
    if (!msg_ptr->get()) msg_ptr->reset(new Message());
    // Receiving a Message from multiple recv
    // Log::Debug("MPI-Net: rank %d started recv msg\n", rank());
    MessagePtr& msg = *msg_ptr;
    msg->data().clear();
    MPI_Status status;
    MPI_Recv(msg->header(), Message::kHeaderSize,
      MPI_BYTE, MPI_ANY_SOURCE,
      0, MPI_COMM_WORLD, &status);
    size_t size = Message::kHeaderSize;
    int i = 0;
    int num_probe = 0;
    while (true) {
      int count;
      CHECK(MPI_SUCCESS == MPI_Probe(msg->src(), 0, MPI_COMM_WORLD, &status));
      //CHECK(MPI_SUCCESS == MPI_Iprobe(msg->src(), 0, MPI_COMM_WORLD, &flag, &status));
      //if (!flag) {
      //  if (num_probe > 100) Log::Debug(" VLOG(RECV), Iprobe failed too much time \n", ++num_probe);
      //  continue;
      //}
      MPI_Get_count(&status, MPI_BYTE, &count);
      Blob blob(count);
      // We only receive from msg->src() until we recv the overtag msg
      MPI_Recv(blob.data(), count, MPI_BYTE, msg->src(),
        0, MPI_COMM_WORLD, &status);
      size += count;
      if (count == sizeof(char)) {
        if (blob.As<char>() == more_) break;
        CHECK(1 + 1 != 2);
      }
      msg->Push(blob);
      // Log::Debug("      VLOG(RECV): i = %d\n", ++i);
    }
    // Log::Debug("MPI-Net: rank %d end recv from src %d, size = %d\n", rank(), msg->src(), size);
    return size;
  }

private:
  const char more_;
  std::mutex mutex_;
  int thread_provided_;
  int inited_;
  int rank_;
  int size_;
};

}

#endif // MULTIVERSO_USE_MPI

#endif // MULTIVERSO_NET_MPI_NET_H_