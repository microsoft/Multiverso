#ifndef MULTIVERSO_LOCAL_FILE_SYS_H_
#define MULTIVERSO_LOCAL_FILE_SYS_H_

/*!
 * \file local_file_sys.h
 * \brief the implement of local io interface.
 */

#include "io.h"

namespace multiverso
{
class LocalStream : public Stream
{
public:
    explicit LocalStream(FILE * fp, std::string path);

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

    ~LocalStream();

private:
    FILE *fp_;
    std::string path_;
};

class LocalFileSystem : public FileSystem
{
public:
    LocalFileSystem(std::string host);
    ~LocalFileSystem(void);

    /*!
    * \brief create a Stream
    * \param path the path of the file
    * \param mode "w" - create an empty file to store data;
    *             "a" - open the file to append data to it
    *             "r" - open the file to read
    * \return the Stream which is used to write or read data
    */
    virtual Stream* Open(const std::string path,
        const char *mode) override;

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

    virtual FileInfo* GetPathInfo(const std::string path) override;

    virtual void Rename(const std::string old_path, const std::string new_path) override;

    virtual void Copy(const std::string src, const std::string dst) override;

    virtual void ListDirectory(const std::string path, std::vector<FileInfo*> & files) override;

private:
    std::string host_;
};
}

#endif // MULTIVERSO_LOCAL_FILE_SYS_H_