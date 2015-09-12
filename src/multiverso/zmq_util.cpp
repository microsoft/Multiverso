#include "msg_pack.h"
#include "zmq_util.h"

namespace multiverso
{
    // Initializes the ZMQ context
    zmq::context_t ZMQUtil::zmq_context_(kZMQ_IO_THREAD_NUM);

    zmq::socket_t *ZMQUtil::CreateSocket()
    {
        zmq::socket_t *socket = new zmq::socket_t(zmq_context_, ZMQ_DEALER);
        socket->connect(kCOMM_ENDPOINT.c_str());
        return socket;
    }

    void ZMQUtil::ZMQPoll(zmq::pollitem_t *items, int count, 
        std::vector<zmq::socket_t*> &sockets,
        std::queue<std::shared_ptr<MsgPack>> &msg_queue)
    {
        if (zmq::poll(items, count, kZMQ_POLL_TIMEOUT) > 0)
        {
            for (int i = 0; i < count; ++i)
            {
                if (items[i].revents & ZMQ_POLLIN)
                {
                    std::shared_ptr<MsgPack> msg_pack(new MsgPack(sockets[i]));
                    msg_queue.push(msg_pack);
                }
            } // end for
        } // end if
    }
}
