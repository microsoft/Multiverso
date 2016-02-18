#ifndef MULTIVERSO_NET_ZMQ_NET_H_
#define MULTIVERSO_NET_ZMQ_NET_H_

#define MULTIVERSO_USE_ZEROMQ
#ifdef MULTIVERSO_USE_ZEROMQ

#include "multiverso/net.h"

#include <limits>
#include "multiverso/util/log.h"
#include "multiverso/message.h"

#include <zmq.h>

namespace multiverso {
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
    ParseMachineFile(argv[0], &machine_lists);
    size_ = static_cast<int>(machine_lists.size());
    for (auto ip : machine_lists) {
      void* requester = zmq_socket(context_, ZMQ_REQ);
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
    size_t size = 0;
    int dst = msg->dst();
    void* socket = requester_[dst];
    int send_size;
    send_size = zmq_send(socket, msg->header(), Message::kHeaderSize, ZMQ_SNDMORE);
    CHECK(Message::kHeaderSize == send_size);
    size += send_size;
    for (size_t i = 0; i < msg->data().size(); ++i) {
      Blob blob = msg->data()[i];
      size_t blob_size = blob.size();
      CHECK_NOTNULL(blob.data());
      send_size = zmq_send(socket, &blob_size, sizeof(size_t), ZMQ_SNDMORE);
      CHECK(send_size == sizeof(size_t));
      send_size = zmq_send(socket, blob.data(), static_cast<int>(blob.size()),
        i == msg->data().size() - 1 ? 0 : ZMQ_SNDMORE);
      CHECK(send_size == blob_size);
      size += blob_size + sizeof(size_t);
    }
    // Send an extra over tag indicating the finish of this Message
    Log::Debug("ZMQ-Net: rank %d send msg size = %d\n", rank(), size);
    return size;
  }

  size_t Recv(MessagePtr* msg_ptr) override {
    size_t size = 0;
    int recv_size, more;
    size_t blob_size, more_size;
    // Receiving a Message from multiple recv
    CHECK_NOTNULL(msg_ptr);
    MessagePtr& msg = *msg_ptr;
    recv_size = zmq_recv(responder_, msg->header(), Message::kHeaderSize, 0);
    CHECK(Message::kHeaderSize == recv_size);
    size += recv_size;
    while (true) {
      zmq_getsockopt(responder_, ZMQ_RCVMORE, &more, &more_size);
      CHECK(more);
      recv_size = zmq_recv(responder_, &blob_size, sizeof(size_t), 0);
      size += recv_size;
      CHECK(recv_size == sizeof(size_t));
      Blob blob(blob_size);
      recv_size = zmq_recv(responder_, blob.data(), blob.size(), 0);
      size += recv_size;
      CHECK(recv_size == blob_size);
      msg->Push(blob);
      zmq_getsockopt(responder_, ZMQ_RCVMORE, &more, &more_size);
      if (!more) break;
    }
    return size;
  }

private:
  void ParseMachineFile(std::string filename, 
                        std::vector<std::string>* result) {
    FILE* file;
#ifdef _MSC_VER
    fopen_s(&file, filename.c_str(), "r");
#else
    file = fopen(filename.c_str(), "r");
#endif
    CHECK_NOTNULL(file);
    // while (fscanf(file, "%s"))
  }


  void* context_;
  void* responder_;
  std::vector<void*> requester_;
  const int more_;
  int inited_;
  int rank_;
  int size_;
};
} // namespace multiverso

#endif // MULTIVERSO_USE_ZEROMQ

#endif // MULTIVERSO_NET_ZMQ_NET_H_