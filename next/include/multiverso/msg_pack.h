#ifndef MULTIVERSO_MSG_PACK_H_
#define MULTIVERSO_MSG_PACK_H_

/*!
 * \file msg_pack.h
 * \brief Defines the MsgPack class.
 * \author: feiyan
 */

#include <vector>
#include "meta.h"

namespace zmq
{
    class message_t;
    class socket_t;
}

namespace multiverso
{
    /*! 
     * \brief The enumeration class MsgArrow defines two orientations of
     *        a message package (MsgPack).
     */
    enum class MsgArrow : int
    {
        Worker2Server = 0, // worker -> server
        Server2Worker = 1  // server -> worker
    };

    /*! 
     * \brief The class MsgPack is a wrapper (container) of ZMQ multiple message.
     *         In addition to address messages and delimiter message (empty 
     *         message), the first data message (indexed with 0) is a header
     *         including the basic information of message type and message arrow.
     *         The following are user customized data message (if there is any).
     */
    class MsgPack
    {
    public:
        /*!\brief Creates an empty MsgPack.*/
        MsgPack();
        /*!
         * \brief Creates a MsgPack with header information.
         * \param type Message type
         * \param arrow Message arrow
         * \param src Source id, worker processor rank or server id dependently
         * \param dst Destination id, server id or processor rank dependently
         */
        MsgPack(MsgType type, MsgArrow arrow, int src, int dst);
        /*!
         * \brief Gets multiple messages from the socket and wraps them in the 
         *        MsgPack.
         * \param socket A ZMQ socket getting the messages from
         */
        explicit MsgPack(zmq::socket_t *socket);
        /*! 
         * \brief Deserializes a block of binary data as ZMQ messages and 
         * composes a MsgPack.
         * \param buffer Pointer to the input memory block
         * \param size Size of the memory block (in bytes)
         */
        MsgPack(char *buffer, int size);
        ~MsgPack();

        /*!
         * \brief Pushes a ZMQ message into the MsgPack.
         * \param msg A pointer to a ZMQ message.
         */
        void Push(zmq::message_t *msg);

        /*!
         * \brief Gets the header information
         * \param type Message type
         * \param arrow Message arrow
         * \param src Source id
         * \param dst Destination id
         */
        void GetHeaderInfo(MsgType *type, MsgArrow *arrow, int *src, int *dst);

        /*!
         * \brief Sends the messages with the socket.
         * \param socket A ZMQ socket for sending the messages
         */
        void Send(zmq::socket_t *socket);

        /*!
         * \brief Returns a pointer to a specific message. (The header message 
         *        is indexed 0) and following are user data message.
         * \param idx Message index.
         */
        zmq::message_t *GetMsg(int idx) { return messages_[start_ + idx]; }

        /*! \brief Returns the number of data messages. */
        int Size() { return static_cast<int>(messages_.size() - start_); }

        /*! 
         * \brief Creates a MsgPack replying to this MsgPack, the router 
         *        addresses (and the delimiter) will be copied to the new 
         *        MsgPack and the header information will be constructed 
         *        accordingly.
         */
        MsgPack *CreateReplyMsgPack();

        /*! 
         * \brief Serializes the messages into a binary block.
         * \param buffer Pointer to the output block
         * \param size of the serialized data block
         */
        void Serialize(char *buffer, int *size);

    private:
        int start_; // index to the header message
        std::vector<zmq::message_t*> messages_;

        // No copying allowed
        MsgPack(const MsgPack&);
        void operator=(const MsgPack&);
    };
}

#endif // MULTIVERSO_MSG_PACK_H_
