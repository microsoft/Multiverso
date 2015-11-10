#ifndef MULTIVERSO_ROW_H_
#define MULTIVERSO_ROW_H_

/*!
 * \file row.h
 * \brief Row container for parameter storage
 * \author feiga, feiyan
 */

#include "meta.h"

namespace multiverso
{
    class RowBaseIterator;

    /*! \brief Interface of row */
    class RowBase
    {
    public:
        typedef RowBaseIterator iterator;
        virtual ~RowBase() {};
        /*!
         * \brief Adds delta to the row value of related key
         * \param key key of row
         * \param delta pointer to delta
         * \return sucess or not, 0: success, -1: error
         */
        virtual int Add(integer_t key, void *delta) = 0;
        /*! 
         * \brief Add an entire row to the row, only applied for DENSE row.
         * \param delta An array pointing to the delta row.
         * \return Returns 0 on success or -1 when error occurs.
         */
        virtual int Add(void *delta) = 0;
        /*!
         * \brief Gets row value of related key
         * \param key key of row
         * \param value pointer to returned value
         */
        virtual void At(integer_t key, void *value) = 0;
        /**
         * \brief Gets capacity of row. Capacity is the maximum number of 
         *        key-value that row can hold
         * \return capacity of row
         */
        virtual integer_t Capacity() = 0;
        /*!
         * \brief Gets the number of non-zero key-value pairs 
         * \return non-zero size of row
         */
        virtual integer_t NonzeroSize() = 0;
        /*!
         * \brief Clears the current row
         */
        virtual void Clear() = 0;
        /*!
         * \brief Gets row id
         * \return row id
         */
        virtual integer_t RowId() = 0;
        /*!
         * \brief Gets format of row. Format is either DENSE or SPARSE
         * \return format of row
         */
        virtual Format RowFormat() = 0;
        /*!
         * \brief Creates an iterator for the row
         * \return pointer to iterator
         */
        virtual iterator *BaseIterator() = 0;
        /*!
         * \brief Serialize a row to a memory
         * \param byte memory to store the serialized row
         */
        virtual void Serialize(void* byte) = 0;
        /*!
         * \brief Deserialize the row from byte and then add it into row
         * \param byte memory to store the serialized row
         */
        virtual void BatchAdd(void* byte) = 0;
        /*!
         * \brief Serialize a row to a string with format 
         *        row_id col_id:col_val col_id:col_val ...
         * \return serialized string
         */
        virtual std::string ToString() = 0;
    };

    template <typename T>
    class RowIterator;

    /*!
     * \brief A hybrid row, can be either DENSE row or SPARSE row
     */
    template <typename T>
    class Row : public RowBase
    {
    public:
        typedef RowIterator<T> iterator;
        friend class RowIterator<T>;

        /*!
         * \brief Constructs a row based on row_id, format, capacity, memory
         * \param row_id id of current row
         * \param format format of current row, either DENSE or SPARSE
         * \param capacity capacity of current row
         * \param memory pointer to the memory block 
         */
        Row(integer_t row_id, Format format, integer_t capacity, 
            void *memory = nullptr);
        ~Row();

        int Add(integer_t key, void *delta) override;
        int Add(integer_t key, T delta);
        int Add(void *delta) override;
        void At(integer_t key, void *value) override;
        T At(integer_t key);

        integer_t Capacity() override;
        integer_t NonzeroSize() override;
        void Clear() override;
        integer_t RowId() override;
        Format RowFormat() override;

        RowBase::iterator *BaseIterator() override;
        iterator Iterator();

        void Serialize(void* byte) override;
        void BatchAdd(void* byte) override;
        std::string ToString() override;
    private:
        /*!
         * \brief Marks key bucket for empty key and deleted key. 
         *        This is only for sparse row. the actual valid key(col id) 
         *        is not less than 0, we set empty key as 0 for efficience 
         *        consideration. Thus in implementation, we would use 
         *        internal key, which equals the actual key plus one
         */
        enum KeyType 
        {
            kEmptyKey = 0,
            kDeletedKey = -1
        };
        static const integer_t kDefaultCapacity = 128;
        /*!
         * \brief Finds position to insert/query, only for sparse row
         * \param key key to be found
         * \param bucket store the result into bucket
         * \return whether found or not
         */
        bool FindPosition(const integer_t key, integer_t& bucket);
        /*!
         * \brief Rehash for sparse row.
         */
        void Rehash(bool resize = false);
	private:
        /*! \brief row id of current row */
		integer_t row_id_;
        /*! \brief format of current row, should be sparse or dense */
		Format format_;
        /*! \brief capacity is the maximum number of key-values current 
        memory block can hold */
		integer_t capacity_;
        /*! \brief number of element */
        integer_t nonzero_size_;
        /*! \brief number of deleted key, if deleted key is too much, 
        we need to re-hash */
        integer_t num_deleted_key_;
        /*! \brief whether current row has its own memory */
        bool has_memory_;
        /*! \brief pointer to memory */
		void* memory_;
        /*! \brief used only by sparse row, store hash keys */
        integer_t* key_;
        /*! \brief value of related key(sparse) or related index(dense) */
        T* value_;

        // No copying allowed
        Row(const Row<T>&);
        void operator=(const Row<T>&);
    };

    class RowFactoryBase
    {
    public:
        virtual ~RowFactoryBase() {}
        virtual RowBase *CreateRow(integer_t row_id, Format format, 
            integer_t capacity, void *memory) = 0;
        virtual int ElementSize() = 0;

        static RowFactoryBase *CreateRowFactory(Type type);
    };

    template <typename T>
    class RowFactory : public RowFactoryBase
    {
    public:
        RowFactory() {}
        ~RowFactory() {}

        RowBase *CreateRow(integer_t row_id, Format format, integer_t capacity,
            void *memory) override;
        int ElementSize() override { return static_cast<int>(sizeof(T)); }
    };
}

#endif // MULTIVERSO_ROW_H_
