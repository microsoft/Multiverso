#ifndef MULTIVERSO_COMMUNICATOR_H_
#define MULTIVERSO_COMMUNICATOR_H_

/*!
 * \file communicator.h
 * \brief Define the Communicator class
 * \author feiyan
 */

#include <string>
#include <vector>
#include <thread>
#include "zmq.hpp"
#include "meta.h"

namespace multiverso
{
    struct Config;
    class Server;

    /*! 
     * \brief The Communicator class is responsible for receiving messages from
     *        internal threads or external processes and forward them to 
     *        appropriate destinations.
     */
    class Communicator
    {
    public:
        /*!
         * \brief Creates a Communicator object and starts a background
         *        communication thread.
         * \param config Configuration information
         * \param argc Number of commandline arguments, for initializing MPI
         * \param argv Commandline arguments, for initializing MPI
         */
        Communicator(const Config &config, int *argc, char **argv[]);
        /*! \brief Cleans up and closes the communicator (thread). */
        ~Communicator();

        /*! 
         * \brief Registers the worker process to the master server, gets the 
         *        assigned process rank and global information.
         * \param socket The main thread socket connecting to the communicator.
         * \param config Multiverso configuration.
         * \param num_trainers Number of local trainers.
         * \return Returns the received register information.
         */
        RegisterInfo Register(zmq::socket_t *socket, const Config &config, 
            int num_trainers);

        ///*! 
        // * \brief Returns MPI rank of the process in MPI version. Undefined
        // *        behavior in non-MPI version.
        // */
        //int MPIRank();
        ///*! 
        // * \brief Returns the number of processes in MPI version. Undefined
        // *        behavior in non-MPI version.
        // */
        //int MPISize();

        /*! 
         * \brief Sets a flag of whether finalizing MPI while closing the
         * Communicator. It is true by default.
         */
        void SetMPIFinalize(bool finalize) { mpi_finalize_ = finalize; }
        ///*! \brief Sets the process rank.*/
        //void SetProcRank(int proc_rank) { proc_rank_ = proc_rank; }
        /*! 
         * \brief Returns a ZMQ DEALER socket for internal working threads 
         *        connecting to the ROUTER socket in the Communicator.
         */
        zmq::socket_t *CreateSocket();
        ///*!
        // * \brief Returns the number of connected servers in ZMQ version. 
        // *        Undefined behavior in MPI version. 
        // */
        //int ServerCount() { return static_cast<int>(dealers_.size()); }

    private:
        void StartThread(std::string server_endpoint_file); // comm thread method
        void Init(std::string server_endpoint_file);        // initialization
        void Clear();                                       // clean up

        // area of member variables ------------------------------------------/
        zmq::socket_t *router_; // socket for receiving working thread messages
        std::vector<zmq::socket_t*> dealers_; // socket for connecting to servers
        zmq::pollitem_t *poll_items_;
        int poll_count_;

        RegisterInfo reg_info_;
        //int proc_rank_;     // process rank
        bool mpi_finalize_; // finalize MPI while closing the communicator if true
        bool is_working_;   // communication thread keeps working while it is true
        std::thread comm_thread_;

        Server *server_;

        // No copying allowed
        Communicator(const Communicator&);
        void operator=(const Communicator&);
    };
}

#endif // MULTIVERSO_COMMUNICATOR_H_
