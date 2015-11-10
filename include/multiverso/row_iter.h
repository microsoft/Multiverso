#ifndef MULTIVERSO_ROW_ITER_H_
#define MULTIVERSO_ROW_ITER_H_

/*!
* \file row.h
* \brief Row iterator provides method to traverse a row
*/

#include "meta.h"

namespace multiverso
{
    /*!
     * \brief Interface of row iterator
     */
    class RowBaseIterator
    {
    public:
        virtual ~RowBaseIterator() {};
        /*!
         * \brief Judges if iterates over the last element
         * \return Retruns true if the current iterator pointing
         *         to a valid element, or false if it points to the end.
         */
        virtual bool HasNext() = 0;
        /*!
         * \brief Iterates to the next element
         */
        virtual void Next() = 0;
        /*!
         * \brief Gets key of current element
         * \return key of current element
         */
        virtual integer_t Key() = 0;
        /*!
         * \brief Gets value of current element
         * \param pointer to returned value
         */
        virtual void Value(void *value) = 0;
    };


    template <typename T>
    class Row;

    template <typename T>
    class RowIterator : public RowBaseIterator
    {
    public:
        /*!
         * \brief Constructs a row iterator
         * \param row to create iterator
         */
        explicit RowIterator(Row<T> &row);
        ~RowIterator();

        bool HasNext() override;
        void Next() override;

        integer_t Key() override;
        void Value(void *value) override;
        T Value();
    private:
        void SkipInvalidValue();
    private:
        /*! \brief row reference of parent row */
        Row<T>& row_;
        /*! \brief current index */
        integer_t index_;
    };
}

#endif // MULTIVERSO_ROW_ITER_H_
