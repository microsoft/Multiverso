#ifndef MULTIVERSO_IO_H_
#define MULTIVERSO_IO_H_

/*!
 * \file io.h
 * \brief Defines io interface.
 */

#include <vector>

namespace multiverso
{

/*!
 * \brief the reference positon for seeking
 * the position point of Stream
 */
enum SeekOrigin
{
    kBegin, kCurrent, kEnd
};

class Stream
{
    /*!
    * \brief move the position point to seekOrigin + offset
    * \param offset the offset(bytes number) to change the position point
    * \param seekOrigin the reference position
    */
    virtual void Seek(size_t offset, SeekOrigin seekOrigin) = 0;
};

/*! \brief use to write binary data to a file*/
class BinaryWriter : public Stream
{
public:
    /*!
    * \brief write data to a file
    * \param buf pointer to a memory buffer
    * \param size data size
    */
    virtual void Write(const void *buf, size_t size) = 0;

    /*!
    * \brief write data to a file
    * \param data pointer to the data to be written
    * \tparam T the data type to be written
    */
    template<typename T>
    virtual void Write(const T *data) = 0;

    /*!
    * \brief flush local buffer
    */
    virtual void Flush();

    virtual ~BinaryWriter(void) {}
}；

class BinaryReader : public Stream
{
public:
    virtual size_t Read(void *buf, size_t size) ＝ 0;

    template<typename T>
    virtual bool Read(T *out_data) = 0;

    virtual ~BinaryReader(void) {}
}；

class TextWriter : public BinaryWriter
{
public:
    virtual void WriteLine(void *buf, size_t size);

    virtual ~TextWriter(void) {}
}；

class TextReader : public BinaryReader
{
public:
    /*!
    * \brief read characters until read '\n' or exceed size
    * \param buf the buffer to store text
    * \param size the size of buf
    * \return the length of text data
    */
    virtual size_t ReadLine(void *buf, size_t size);

    virtual ~TextReader(void) {}
}；

enum FileType
{
    kFile,
    kDirectory
};

struct FileInfo
{
    FileInfo() : size(0), type(kFile) {}
    std::string path;
    size_t size;
    FileType type;
};

class FileSystem
{
public:
    /*!
    * /brief the constro
    */
    FileSystem(const std::string protocol, const std::string hostname);

    virtual BinaryReader CreateBinaryReader(const std::string path) = 0;

    /*!
    * \brief create a BinaryWriter to write binary data to the file(path)
    * \param path the path of the file
    * \param mode "w" - create an empty file; "a" - open the file to append data to it 
    * \return the BinaryWriter which is used to write data to the file(path)
    */
    virtual BinaryWriter CreateBinaryWriter(const std::string path,
        const char *mode = "w") = 0;

    virtual TextReader CreateTextReader(const std::string path) = 0;

    /*!
    * \brief create a TextWriter to write text data to the file(path)
    * \param path the path of the file
    * \param mode "w" - create an empty file; "a" - open the file to append data to it 
    * \return the TextWriter which is used to write data to the file(path)
    */
    virtual TextWriter CreateTextWriter(const std::string path,
        const char *mode = "w") = 0;

    virtual bool Exists(const std::string path) = 0;

    virtual void Delete(const std::string path) = 0;

    virtual FileInfo GetPathInfo(const std::string path) = 0;

    virtual void Rename(const std::string old_path, const std::string new_path) = 0;

    virtual void Copy(const std::string src, const std::string dst) = 0;

    virtual void GetFiles(const std::string path, std::std::vector<FileInfo> files) = 0;

    virtual ~FileSystem(void) {}
};
}

#endif // MULTIVERSO_IO_H_
