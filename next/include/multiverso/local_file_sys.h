#ifndef MULTIVERSO_LOCAL_FILE_SYS_H_
#define MULTIVERSO_LOCAL_FILE_SYS_H_

/*!
 * \file local_file_sys.h
 * \brief Defines the implement of local io interface.
 */

#include "io.h"

 namespace multiverso
{
class LocalStream : public LocalStream
{
    /*!
    * \brief move the position point to seekOrigin + offset
    * \param offset the offset(bytes number) to change the position point
    * \param seekOrigin the reference position
    */
    void Seek(size_t offset, SeekOrigin seekOrigin) override;
};

class LocalBinaryReader : public LocalStream, public BinaryReader
{

};

class LocalBinaryWriter : public LocalStream, public BinaryWriter
{
public:
    /*!
    * \brief write data to a file
    * \param buf pointer to a memory buffer
    * \param size data size
    */
    void Write(const void *buf, size_t size) override;

    /*!
    * \brief write data to a file
    * \param data pointer to the data to be written
    * \tparam T the data type to be written
    */
    template<typename T>
    void Write(const T *data) override;

    /*!
    * \brief move the position point to seekOrigin + offset
    * \param offset the offset(bytes number) to change the position point
    * \param seekOrigin the reference position
    */
    void Seek(size_t offset, SeekOrigin seekOrigin) override;

    /*!
    * \brief flush local buffer
    */
    void Flush() override;


    ~LocalBinaryWriter(void) {}
};

class LocalTextReader : public LocalBinaryReader, public TextReader
{

};

class LocalTextWriter : public LocalBinaryWriter, public TextWriter
{

};
}

#endif // MULTIVERSO_LOCAL_FILE_SYS_H_