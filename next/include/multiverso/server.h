#ifndef MULTIVERSO_SERVER_H_
#define MULTIVERSO_SERVER_H_

/*!
 * \file server.h
 * \brief Defines the Server class
 * \author feiyan, feiga
 */

#include <memory>
#include <vector>
#include <string>
#include <thread>
#include "zmq.hpp"
#include "meta.h"
#include "mt_queue.h"
#include "lock.h"

namespace multiverso
{
    class Table;
    class MsgPack;

    /*!
     * \brief The server class is responsible for storing the global model 
     *        parameters in a distributed manner. One application could have
     *        one or more Server instances with different IDs. And the 
     *        parameters will be distributed among such servers.
     */
    class Server
    {
    public:
        /*!
         * \brief Creates a server instance and starts a server thread.
         * \param server_id Identity of server
         * \param num_worker_process Total number of worker processes
         * \param endpoint The communication ZQM socket endpoint
         */
        Server(int server_id, int num_worker_process, std::string endpoint);
        /*! \brief Destroies the server instance and terminates the server thread.*/
        ~Server();

        /*! brief Blocks current threads and waits for the server thread completed.*/
        void WaitToComplete();

    private:
        void StartThread();        // server thread method
        void StartUpdateThread();  // server update thread method
        void Init();   // initialization at the beginning of server thread
        void Clear();  // clean up at the end of server thread

        // message processing rountine
        void Process_Register(std::shared_ptr<MsgPack> msg_pack);
        void Process_Close(std::shared_ptr<MsgPack> msg_pack);
        // Returns true if it is the last one comes, or false otherwise
        bool Process_Barrier(std::shared_ptr<MsgPack> msg_pack);
        void Process_CreateTable(std::shared_ptr<MsgPack> msg_pack);
        void Process_SetRow(std::shared_ptr<MsgPack> msg_pack);
        void Process_Clock(std::shared_ptr<MsgPack> msg_pack);
        void Process_EndTrain(std::shared_ptr<MsgPack> msg_pack);
		void Process_Get(std::shared_ptr<MsgPack> msg_pack);
        void Process_Add(std::shared_ptr<MsgPack> msg_pack);

        // helper function for processing get row request
        void Process_GetRow(integer_t table, integer_t row, integer_t col,
            std::shared_ptr<MsgPack> msg_pack, 
            int& send_ret_size, MsgPack*& reply);
        
        void DumpModel();
        // area of member variables ------------------------------------------/
        int server_id_;             // server identity
        int worker_proc_count_;     // total number of worker processes
        std::string endpoint_;
        zmq::socket_t *router_;
        zmq::pollitem_t *poll_items_;
        int poll_count_;
        std::vector<std::shared_ptr<MsgPack>> waiting_msg_;

        bool is_working_;
        bool inited_;
        std::thread server_thread_;

        std::thread update_thread_;

        int max_delay_;
        std::vector<int> clocks_;
        std::vector<std::shared_ptr<MsgPack>> clock_msg_;
        std::vector<Table*> tables_;

        MtQueueMove<std::shared_ptr<MsgPack>> update_queue_;
        LockManager lock_pool_;
        // No copying allowed
        Server(const Server&);
        void operator=(const Server&);
    };
}

#endif // MULTIVERSO_SERVER_H_
