#ifndef MULTIVERSO_ZMQ_UTIL_H_
#define MULTIVERSO_ZMQ_UTIL_H_

/*!
 * \file zmq_util.h
 * \brief Wraps the ZMQ facilities
 * \author feiyan
 */

#include <string>
#include <vector>
#include <queue>
#include <memory>
#include "zmq.hpp"

namespace multiverso
{
    const std::string kCOMM_ENDPOINT = "inproc://comm";     // communicator endpoint
    const std::string kSERVER_ENDPOINT = "inproc://server"; // server endpoint
    const int kZMQ_IO_THREAD_NUM = 1;
    const int kZMQ_POLL_TIMEOUT = 0;

    class MsgPack;

    class ZMQUtil
    {
    public:
        /*! 
         * \brief Creates and returns a ZMQ DEALER socket connecting to 
         *        communication end point.
         */
        static zmq::socket_t *CreateSocket();

        /*! 
         * \brief Returns a reference to the ZMQ contest, mainly for creating 
         *        sockets. 
         */
        static zmq::context_t &GetZMQContext() { return zmq_context_; }

        /*! 
         * \brief Polls a group of ZMQ sockets to probe if there are received 
         *        messages. If so, pushes them into the output queue.
         * \param items ZMQ poll item array
         * \param count Size of the item array
         * \param sockets The corresponding sockets (with same order in items)
         * \param msg_queue Queue for receiving the messages
         */
        static void ZMQPoll(zmq::pollitem_t *items, int count,
            std::vector<zmq::socket_t*> &sockets,
            std::queue<std::shared_ptr<MsgPack>> &msg_queue);

    private:
        static zmq::context_t zmq_context_;
    };
}

#endif // MULTIVERSO_ZMQ_UTIL_H_
