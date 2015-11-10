#ifndef MULTIVERSO_PARAMETER_LOADER_H_
#define MULTIVERSO_PARAMETER_LOADER_H_

/*!
 * \file parameter_loader.h
 * \brief Defines parameter loader.
 */

#include "meta.h"
#include "mt_queue.h"

#include <thread>
#include <unordered_set>

namespace zmq
{
    class socket_t;
}

namespace multiverso
{
    /*! \brief Tuple represets the parameters users request */
    struct Tuple
    {
        integer_t table;
        integer_t row;
        integer_t col;
        bool operator==(const Tuple& other) const
        {
            return (table == other.table
                && row == other.row
                && col == other.col);
        }
    };
}

namespace std
{
    /*! 
     * \brief Explicity template specializations for std::hash<> in order to 
     *  use Tuple as key of unordered- container
     */
    template<> struct hash<multiverso::Tuple>
    {
        size_t operator()(const multiverso::Tuple& t)const
        {
            return hash<multiverso::integer_t>()(t.table) ^
                hash<multiverso::integer_t>()(t.row) ^
                hash<multiverso::integer_t>()(t.col);
        }
    };
}

namespace multiverso
{
    class DataBlockBase;

    /*!
     * \brief ParameterLoaderBase handles the prameter request through a 
     *  background thread.
     */
    class ParameterLoaderBase
    {
    public:
        ParameterLoaderBase();
        virtual ~ParameterLoaderBase();
        /*!
         * \brief Push a data block to loader, loader would get related 
         *  parameter.
         */
        void PushDataBlock(DataBlockBase *data_block);
        /*!
         * \brief Start a parameter loader thread
         */
        void Start();
        /*!
         * \brief Stop the background parameter loader thread
         */
        void Stop();
        /*!
         * \brief Parses a data block to figure out the parameters needed to
         *  train this data block. Users should implement and override
         *  this function.
         * \prama data_block data block to parse
         */
        virtual void ParseAndRequest(DataBlockBase *data_block);
        /*!
         * \brief Requests a table from server
         * \param table table id
         */
        void RequestTable(integer_t table);
        /*!
         * \brief Requests a row from server
         * \param table table id
         * \param row row id
         */
        void RequestRow(integer_t table, integer_t row);
        /*!
         * \brief Requests a element from server
         * \param table table id
         * \param row row id
         * \param col col id
         */
        void RequestElement(integer_t table, integer_t row, integer_t col);

        /*! \brief underlying implementation of parameter request */
        void ProcessRequest();
       
        void BeginIteration();
        void EndIteration();
    private:
        /*! \brief The entrance function of backgroud thread */
        void StartThread();

        /*! \brief queue storing data for parse and request */
        MtQueueMove<DataBlockBase*> data_queue_;
        /*! \brief Set of requested Tuple */
        std::unordered_set<Tuple> requests_;
        std::thread loader_thread_;

        // No copying allowed
        ParameterLoaderBase(const ParameterLoaderBase&);
        void operator=(const ParameterLoaderBase&);
    };
}

#endif // MULTIVERSO_PARAMETER_LOADER_H_
