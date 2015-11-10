#ifndef MULTIVERSO_TABLE_H_
#define MULTIVERSO_TABLE_H_

/*!
 * \file table.h
 * \brief Table declaration.
 */

#include "meta.h"

#include <mutex>
#include <vector>

namespace multiverso
{
    struct RowInfo
    {
    public:
        RowInfo(Format _format, integer_t _capacity, integer_t _index = -1);

        Format format;
        integer_t capacity;
        integer_t index;
        RowBase* row;
    };

    class RowBase;
    class TableIterator;
	class RowFactoryBase;

    /*! \brief The class Table is a container of rows. */
    class Table
    {
    public:
        typedef TableIterator iterator;
		friend class TableIterator;

        /*! 
         * \beirf Creates a table.
         * \param table_id Table identity
         * \param rows Number of rows
         * \param cols Number of columns
         * \param Element type
         * \param default_format Default row format
         * \param memory_pool_size Memory pool size. When creating rows,
         *        the table will reuse the memory pool if available.
         */
        Table(integer_t table_id, integer_t rows, integer_t cols, Type type, 
            Format default_format, int64_t memory_pool_size = 0);
        ~Table();

        /*!
         * \brief Configures a row.
         * \param row_id Row id
         * \param format Row format.
         * \param capacity The initial maximal number of elements the row 
         *        stores, it is fixed for DENSE row (array), and can be 
         *        auto-growed for SPARSE row (hash table).
         * \return Returns 0 on success, or -1 with error.
         */
        int SetRow(integer_t row_id, Format format, integer_t capacity);
        /*! \brief Returns a pointer to the specific row, or NULL if not avaiable. */
        RowBase *GetRow(integer_t row_id);
        /*! \brief Returns a reference to a specified RowInfo. */
        RowInfo *GetRowInfo(integer_t row_id);

        /*! \brief Removes all rows in the table. */
        void Clear();
        /*! \brief Get element size of current table */
        int ElementSize();

        /*! \brief Returns an iterator for accessing all rows. */
        TableIterator Iterator();

	private:
        integer_t table_id_;
        char *memory_pool_;
        int64_t memory_size_;
        int64_t used_memory_size_;
        RowFactoryBase* row_factory_;
        int element_size_;
        std::vector<RowInfo> row_info_;
        std::vector<RowBase*> rows_;
        std::mutex mutex_;

        // No copying allowed
        Table(const Table&);
        void operator=(const Table&);
    };
}

#endif // MULTIVERSO_TABLE_H_
