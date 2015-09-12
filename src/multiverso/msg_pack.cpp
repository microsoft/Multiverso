#include "zmq.hpp"
#include "msg_pack.h"

namespace multiverso
{
    // Creates an empty MsgPack.
    MsgPack::MsgPack() { start_ = 0; }

    // Creates a MsgPack with header information
    MsgPack::MsgPack(MsgType type, MsgArrow arrow, int src, int dst)
    {
        start_ = 0;
        Push(new zmq::message_t(0));
        zmq::message_t *header = new zmq::message_t(4 * sizeof(int));
        int *buffer = static_cast<int*>(header->data());
        buffer[0] = static_cast<int>(type);
        buffer[1] = static_cast<int>(arrow);
        buffer[2] = src;
        buffer[3] = dst;
        Push(header);
    }

    // Receives a multiple message from the socket and push into this MsgPack
    MsgPack::MsgPack(zmq::socket_t *socket)
    {
        start_ = 0;
        while (true)
        {
            zmq::message_t *msg = new zmq::message_t();
            socket->recv(msg);
            Push(msg);
            if (!msg->more())
            {
                break;
            }
        }
    }

    // Deserializes the data block and composes a MsgPack.
    MsgPack::MsgPack(char *buffer, int size)
    {
        start_ = 0;
        int msg_size = 0;
        for (int pos = 0; pos < size;)
        {
            // get the message size
            memcpy(&msg_size, buffer + pos, sizeof(int));
            pos += sizeof(int);
            // copy the message content
            zmq::message_t *msg = new zmq::message_t(msg_size);
            memcpy(msg->data(), buffer + pos, msg_size);
            Push(msg);
            pos += msg_size;
        }
    }

    MsgPack::~MsgPack()
    {
        for (auto &msg : messages_)
        {
            delete msg;
        }
    }

    // Pushes a message into back of the array
    void MsgPack::Push(zmq::message_t *msg)
    {
        messages_.push_back(msg);
        if (start_ == 0 && msg->size() == 0)
        {
            start_ = static_cast<int>(messages_.size());
        }
    }

    // Copies the header information
    void MsgPack::GetHeaderInfo(MsgType *type, MsgArrow *arrow, int *src, int *dst)
    {
        int *buffer = static_cast<int*>(messages_[start_]->data());
        *type = static_cast<MsgType>(buffer[0]);
        *arrow = static_cast<MsgArrow>(buffer[1]);
        *src = buffer[2];
        *dst = buffer[3];
    }

    // Sends the messages with the socket
    void MsgPack::Send(zmq::socket_t *socket)
    {
        int size = static_cast<int>(messages_.size()), more;
        for (int i = 0; i < size; ++i)
        {
            more = (i < size - 1) ? ZMQ_SNDMORE : 0;
            socket->send(*messages_[i], more);
        }
    }

    // Creates reply message (address + header)
    MsgPack *MsgPack::CreateReplyMsgPack()
    {
        MsgPack *reply = new MsgPack();
        // copy addresses
        for (int i = 0; i < start_; ++i)
        {
            zmq::message_t *addr = new zmq::message_t(messages_[i]->size());
            memcpy(addr->data(), messages_[i]->data(), messages_[i]->size());
            reply->Push(addr);
        }
        // construct the reply header
        int *my_header = static_cast<int*>(messages_[start_]->data());
        zmq::message_t *header = new zmq::message_t(4 * sizeof(int));
        int *buffer = static_cast<int*>(header->data());
        buffer[0] = -my_header[0];    // reply message type
        buffer[1] = 1 - my_header[1]; // worker <--> server
        buffer[2] = my_header[3];     // src <--> dst
        buffer[3] = my_header[2];
        reply->Push(header);
        return reply;
    }

    // Serializes the messages into the binary data block.
    void MsgPack::Serialize(char *buffer, int *size)
    {
        *size = 0;
        char *buf = buffer;
        int64_t count = messages_.size();
        int piece_size = 0;
        for (int64_t i = 0; i < count; ++i)
        {
            // copy the message size first
            piece_size = static_cast<int>(messages_[i]->size());
            memcpy(buf, &piece_size, sizeof(int));
            buf += sizeof(int);
            // then copy the message content
            memcpy(buf, messages_[i]->data(), piece_size);
            buf += piece_size;
            (*size) += piece_size + sizeof(int);
        }
    }
}
