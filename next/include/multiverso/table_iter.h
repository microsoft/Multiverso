#ifndef MULTIVERSO_TABLE_ITER_H_
#define MULTIVERSO_TABLE_ITER_H_

/*!
 * \file table_iter.h
 * \brief Table iterator declaration.
 */

#include "meta.h"

namespace multiverso
{
    class RowBase;
    class Table;

    /*! \brief The class TableIterator is for accessing the rows in Table. */
    class TableIterator
    {
    public:
        /*! \brief Creates an iterator for accessing the table. */
        explicit TableIterator(Table &table);
        ~TableIterator();

        /*! \brief Returns true if current element is valid, 
        or false if it points to the end.*/
        bool HasNext();
        /*!
         * \brief Move the iterator to the next position.
         * \return Returns 0 if moving successfully, or -1 if pointing to the end.
         */
        void Next();

        /*! \brief Returns current row id, or -1 if pointing to the end. */
        integer_t RowId();
        /// Returns a pointer to current row, or NULL if pointing to the end.
        RowBase *Row();
	private:
		/*! \brief Table to iterator */
		Table& table_;
		/*! \brief Current row index */
		integer_t idx_;
    };
}

#endif // MULTIVERSO_TABLE_ITER_H_
