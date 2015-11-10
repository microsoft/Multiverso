#ifndef MULTIVERSO_MULTIVRSO_H_
#define MULTIVERSO_MULTIVRSO_H_

/*!
 * \file multiverso.h
 * \brief Multiverso API declaration. The applications utilizing Multiverso 
 *        should include this header file.
 * \author
 *   - Feidiao Yang (feiyan@microsoft.com)
 *   - Fei Gao (feiga@microsoft.com)
 */

#include <string>
#include <vector>
#include "meta.h"
#include "log.h"
#include "row.h"
#include "row_iter.h"
#include "table.h"
#include "table_iter.h"
#include "data_block.h"
#include "parameter_loader.h"
#include "trainer.h"
#include "stop_watch.h"

namespace zmq
{
    class socket_t;
}

namespace multiverso
{
    class MsgPack;
    class LockManager;
    class Communicator;
    class Aggregator;
    //class Server;
    template <class Container>
    class DoubleBuffer;
    template <typename T>
    class Row;

    class Multiverso
    {
        friend class TrainerBase;
        friend class ParameterLoaderBase;

    public:
        // 1. -- BEGIN: Multiverso global environment API code area --------- //
        /*!
         * \brief Initializes the Multiverso environment.
         * \param trainers Local trainers
         * \param param_loader Parameter loader
         * \param config Multiverso configuration
         * \param argc Number of commandline arguments, for initializing MPI
         * \param argv Commandline arguments, for initializing MPI
         * \return 0 on success, or -1 if there is error
         */
        static int Init(std::vector<TrainerBase*> &trainers, 
            ParameterLoaderBase *param_loader, const Config &config, 
            int *argc, char **argv[]);

        /*!
         * \brief Initializes the Multiverso environment, this version won't 
         *        handle multi-thread logic
         * \param see \ref Init above
         */
        static int Init(const Config &config, int *argc, char **argv[]);

        /*!
         * \brief Closes Multiverso environment.
         * \param finalize Only applied in MPI version. If finalize == true, 
         *        MPI will be closed and users do not able to reuse Multiverso
         *        in another round; or MPI will be kept and uers are allowed to
         *        re-init Multiverso.
         */
        static void Close(bool finalize = true);

        /*! \brief Returns the id (rank) of the worker process. */
        static int ProcessRank() { return reg_info_.proc_rank; }
        /*! \brief Returns the total number of worker processes. */
        static int TotalProcessCount() { return reg_info_.proc_count; }
        /*! \brief Returns the total number of trainers. */
        static int TotalTrainerCount() { return reg_info_.total_trainer_count; }
        /*! \brief Returns the total number of servers. */
        static int TotalServerCount() { return reg_info_.server_count; }
        // -- END: Multiverso global environment API code area -------------- //

        // 2. -- BEGIN: Parameter Server Initialization API code area ------- //
        /*! \brief Beginning of initialization code area. */
        static void BeginConfig();
        
        /*! \brief End of initialization code area. Barrier all worker processes. */
        static void EndConfig();
        /*!
         * \brief Add a logic table to Multiverso environment. This table will
         *        have 3 copies actually: in server, in local cache and in 
         *        aggregator. And the copies in local cache and aggregator have
         *        memory pools with the same size. If you would like them to
         *        have different memory pool size or other options, you will
         *        you will call the specific AddXXXTable(...) method seperately.
         * \param table Identity of the table, indexed from 0
         * \param rows Number of rows
         * \param cols Number of columns
         * \param type Element type
         * \param default_format Default format
         * \param memory_pool_size If greater than 0, a memory pool will be 
         *        created and the table will resue the memory as possible as 
         *        it can
         */
        static void AddTable(integer_t table, integer_t rows, integer_t cols, 
            Type type, Format default_format, int64_t memory_pool_size = 0);

        /*!
         * \brief Sends messages to each server of creating a server table. 
         *        This method should be called in order with respect to 
         *        table id. If multiple workers send messages for the same 
         *        table, the first received message will be applied and the 
         *        followings will be ignored.
         * \param table Identity of table, indexed from 0
         * \param rows Number of rows
         * \param cols Number of columns
         * \param type Element type
         * \param default_format Default format
         */
        static void AddServerTable(integer_t table, integer_t rows, 
            integer_t cols, Type type, Format default_format);
        /*! 
         * \brief Creates a cache table. This method should be called in order
         *        with respect to table id.
         * \param table Identity of table, indexed from 0
         * \param rows Number of rows
         * \param cols Number of columns
         * \param type Element type
         * \param default_format Default format
         * \param memory_pool_size If greater than 0, a memory pool will be 
         *        created and the rows will reuse the memory as possible as it
         *        can.
         * \return Return 0 on success or -1 if error.
         */
        static int AddCacheTable(integer_t table, integer_t rows, integer_t cols, 
            Type type, Format default_format, int64_t memory_pool_size = 0);
        /*! 
         * \brief Creates a aggregator table.
         * \param table Identity of table, indexed from 0
         * \param rows Number of rows
         * \param cols Number of columns
         * \param type Element type
         * \param default_format Default format
         * \param memory_pool_size If greater than 0, a memory pool will be
         *        created and the rows will reuse the memory as possible as it
         *        can.
         */
        static void AddAggregatorTable(integer_t table, integer_t rows, 
            integer_t cols, Type type, Format default_format, 
            int64_t memory_pool_size = 0);

        /*!
         * \brief A unified method of setting a row in server, cache and 
         *        aggregator with identical options. If you would like to set
         *        the row with different settings in these three tables, you
         *        will call the corresponding SetXXXRow(...) methods seperately.
         * \param table Identity of the table
         * \param row Identity of the row
         * \param format Row format
         * \param capacity The maximal number of elements in the row. It is
         *        fixed for DENSE row and could be automatically growed for 
         *        SPARSE row
         */
        static void SetRow(integer_t table, integer_t row, Format format,
            integer_t capacity);

        /*! 
         * \brief Sends a message to the server for configuring a row.
         * \param table Identity of the table
         * \param row Identity of the row
         * \param format Row format
         * \param capacity The maximal number of elements in the row, it could
         *        be automatically growed for SPARSE row
         */
        static void SetServerRow(integer_t table, integer_t row, Format format,
            integer_t capacity);
        /*! 
         * \brief Configures a row in cache.
         * \param table Identity of the table
         * \param row Identity of the row
         * \param format Row format
         * \param capacity The maximal number of elements in the row, it could
         *        be automatically growed for SPARSE row.
         */
        static int SetCacheRow(integer_t table, integer_t row, Format format, 
            integer_t capacity);
        /*! 
         * \brief Configures a row in aggregator.
         * \param table Identity of the table
         * \param row Identity of the row
         * \param format Row format
         * \param capacity The maximal number of elements in the row, it could
         *        be automatically growed for SPARSE row.
         */
        static int SetAggregatorRow(integer_t table, integer_t row, Format format, 
            integer_t capacity);

        template <typename T>
        static void AddToServer(integer_t table, integer_t row, integer_t col,
            T delta)
        {
            AddToServerPtr(table, row, col, &delta);
        }

        /*! \brief Flush the aggregation to sever */
        static void Flush();
        // -- END: Parameter Server Initialization API code area ------------ //

        // 3. -- BEGIN: Data API code area ---------------------------------- //
        /*! \brief Beginning of training code area.*/
        static void BeginTrain();
        /*! \brief End of training code area.*/
        static void EndTrain();
        /*! \brief Begin of each clock period (epoch).*/
        static void BeginClock();
        /*! \brief End of each clock period (epoch).*/
        static void EndClock();
        /*! \brief Pushs a data block into Multiverso and triggers an iteration.*/
        static void PushDataBlock(DataBlockBase *data_block);
        /*! \brief Wait all data block */
        static void Wait();
        // -- END: Data API code area --------------------------------------- //

    private:
        static void FlushSetServerRow();
        static void AddToServerPtr(
            integer_t table, integer_t row, integer_t col, void *delta);

        // area of member variables ------------------------------------------/
        static RegisterInfo reg_info_;
        static int num_trainers_;

        static zmq::socket_t *socket_;
       
        static LockManager *lock_manager_;
        static LockOption lock_option_;
        static Communicator *communicator_;
        static Aggregator *aggregator_;
        
        static bool is_pipeline_;
        static Barrier* pipeline_barrier_;
        static std::vector<TrainerBase*> trainers_;
        static ParameterLoaderBase *param_loader_;

        static std::vector<Table*> tables0_;
        static std::vector<Table*> tables1_;
        static DoubleBuffer<std::vector<Table*>> *double_buffer_;
        
        static std::mutex mutex_;
        static std::condition_variable wait_cv_;
        static std::vector<bool> data_tag_;

        static std::vector<MsgPack*> row_config_;
        static std::vector<int> row_config_size_;
        static int row_config_count_;
    };
}

#endif // MULTIVERSO_MULTIVRSO_H_
