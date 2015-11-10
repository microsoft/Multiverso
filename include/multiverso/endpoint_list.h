#ifndef MULTIVERSO_ENDPOINT_LIST_H_
#define MULTIVERSO_ENDPOINT_LIST_H_

/*
 * \file endpoint_list.h
 * \brief Defines the EndpointList class.
 * \author feiyan
 */

#include <string>
#include <vector>

namespace multiverso
{
    /*! 
     * \brief The EndpointList class is a container for storing server ZMQ 
     *        socket endpoints. It reads the information from a simple 
     *        configuration file. The file contains multiple lines each for a 
     *        server with the format "server_id ip:port"
     */
    class EndpointList
    {
    public:
        /*! 
         * \brief Constructs an EndpointList instance and reads the endpoint 
         *        configuraion from the file.
         * \param filename The filename of the server ZMQ endpoint configuration.
         */
        explicit EndpointList(std::string filename);
        ~EndpointList();

        /*!
         * \brief Returns the endpoint of a specified server id. Returns empty 
         *        string "" if invalid. 
         */
        std::string GetEndpoint(int id);
        /*! \brief Returns the number of endpoints (servers). */
        int Size() { return static_cast<int>(endpoints_.size()); }

    private:
        std::vector<std::string> endpoints_;
    };
}

#endif // MULTIVERSO_ENDPOINT_LIST_H_
