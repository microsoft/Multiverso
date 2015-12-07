#ifndef MULTIVERSO_IO_H_
#define MULTIVERSO_IO_H_

/*!
 * \file io.h
 * \brief Defines io interface.
 */

#include <cstring>
#include <cstdio>
#include <cerrno>

#include <string>
#include <vector>

#include "log.h"

namespace multiverso
{

/*!
 * \brief the reference positon for seeking
 * the position point of Stream
 */
enum class SeekOrigin : int
{
    kBegin = 0,
    kCurrent = 1,
    kEnd = 2
};

class Stream
{
public:

    /*!
    * \brief write data to a file
    * \param buf pointer to a memory buffer
    * \param size data size
    */
    virtual void Write(const void *buf, size_t size) = 0;

    /*!
    * \brief read data from Stream
    * \param buf pointer to a memory buffer
    * \param size the size of buf
    */
    virtual size_t Read(void *buf, size_t size)= 0;

    /*!
    * \brief move the position point to seekOrigin + offset
    * \param offset the offset(bytes number) to change the position point
    * \param seekOrigin the reference position
    */
    virtual void Seek(size_t offset, SeekOrigin seekOrigin) = 0;

    /*!
    * \brief flush local buffer
    */
    virtual void Flush() = 0;
};


enum class FileType : int
{
    kFile = 0,
    kDirectory = 1
};

struct FileInfo
{
    FileInfo() : size(0), type(FileType::kFile) {}
    std::string path;
    size_t size;
    FileType type;
};

class FileSystem
{
public:
    /*!
    * /brief get an instance of FileSystem
    * /param type the type of FileSystem
    *        "hdfs" or "file"
    * /param host for "hdfs", host is address of namenode
    *        for "file", host can be empty
    */
    static FileSystem *GetInstance(const std::string type,
        const std::string host);

    /*!
    * \brief create a Stream
    * \param path the path of the file
    * \param mode "w" - create an empty file to store data;
    *             "a" - open the file to append data to it
    *             "r" - open the file to read
    * \return the Stream which is used to write or read data
    */
    virtual Stream *Open(const std::string path,
        const char *mode) = 0;

    /*!
    * \brief check if the path exists
    * \param path the file or directory
    */
    virtual bool Exists(const std::string path) = 0;

    /*!
    * \brief delete the file or directory
    * \param path the file or directory
    */
    virtual void Delete(const std::string path) = 0;

    virtual FileInfo *GetPathInfo(const std::string path) = 0;

    virtual void Rename(const std::string old_path, const std::string new_path) = 0;

    virtual void Copy(const std::string src, const std::string dst) = 0;

    virtual void ListDirectory(const std::string path, std::vector<FileInfo*> &files) = 0;

protected:
    FileSystem() {}
};
}
#endif // MULTIVERSO_IO_H_
