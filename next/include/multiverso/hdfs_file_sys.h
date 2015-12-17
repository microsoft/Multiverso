#ifndef MULTIVERSO_HDFS_FILE_SYS_H_
#define MULTIVERSO_HDFS_FILE_SYS_H_

/*!
 * \file local_file_sys.h
 * \brief The implement of hdfs io interface.
 */

#include "io.h"

#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cassert>

#include <algorithm>

extern "C" 
{
#include <hdfs.h>
}

 namespace multiverso
{
class HDFSStream : public Stream
{
public:
    HDFSStream(hdfsFS fs, hdfsFile fp, std::string path);

    ~HDFSStream(void);

    /*!
    * \brief write data to a file
    * \param buf pointer to a memory buffer
    * \param size data size
    */
    virtual void Write(const void *buf, size_t size) override;


    /*!
    * \brief read data from Stream
    * \param buf pointer to a memory buffer
    * \param size the size of buf
    */
    virtual size_t Read(void *buf, size_t size) override;

    /*!
    * \brief move the position point to seekOrigin + offset
    * \param offset the offset(bytes number) to change the position point
    * \param seekOrigin the reference position
    */
    virtual void Seek(size_t offset, SeekOrigin seekOrigin) override;

    /*!
    * \brief flush local buffer
    */
    virtual void Flush() override;

private:
    hdfsFS fs_;
    hdfsFile fp_;
    std::string path_;
};

class HDFSFileSystem : public FileSystem
{
public:
    explicit HDFSFileSystem(const std::string host);
    virtual ~HDFSFileSystem(void);

    /*!
    * \brief create a Stream
    * \param path the path of the file
    * \param mode "w" - create an empty file to store data;
    *             "a" - open the file to append data to it
    *             "r" - open the file to read
    * \return the Stream which is used to write or read data
    */
    virtual Stream *Open(const std::string path,
        const char *mode) override;

    virtual void CreateDirectory(const std::string path) override;
    /*!
    * \brief check if the path exists
    * \param path the file or directory
    */
    virtual bool Exists(const std::string path) override;

    /*!
    * \brief delete the file or directory
    * \param path the file or directory
    */
    virtual void Delete(const std::string path) override;

    virtual FileInfo*GetPathInfo(const std::string path) override;

    virtual void Rename(const std::string old_path, const std::string new_path) override;

    virtual void Copy(const std::string src, const std::string dst) override;

    virtual void ListDirectory(const std::string path, std::vector<FileInfo*> &files) override;

    virtual void Close() override;

private:
    std::string namenode_;
    hdfsFS fs_;
};
}
#endif // MULTIVERSO_HDFS_FILE_SYS_H_