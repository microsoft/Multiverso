#ifndef MULTIVERSO_HDFS_FILE_SYS_H_
#define MULTIVERSO_HDFS_FILE_SYS_H_

/*!
* \file local_file_sys.h
* \brief The implement of hdfs io interface.
*/

#include "multiverso/util/io.h"
#include "multiverso/util/hdfs.h"

#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cassert>

#include <algorithm>
#include <memory>


//#define _CONNECT_NEW_INSTANCE_

namespace multiverso
{
  class HDFSStream : public Stream
  {
  public:
    HDFSStream(hdfsFS fs, const URI& uri, const char *mode);

    virtual ~HDFSStream(void) override;

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

    virtual bool Good() override;
    /*!
    * \brief flush local buffer
    */
    virtual void Flush() override;

  private:
    bool is_good_;
    hdfsFS fs_;
    hdfsFile fp_;
    std::string path_;
    std::string mode_;
  };

  class HDFSStreamFactory : public StreamFactory
  {
  public:
    explicit HDFSStreamFactory(const std::string& host);
    virtual ~HDFSStreamFactory(void) override;

    /*!
    * \brief create a Stream
    * \param path the path of the file
    * \param mode "w" - create an empty file to store data;
    *             "a" - open the file to append data to it
    *             "r" - open the file to read
    * \return the Stream which is used to write or read data
    */
    virtual std::shared_ptr<Stream> Open(const URI& uri,
      const char *mode) override;

    virtual void Close() override;

  private:
    std::string namenode_;
    hdfsFS fs_;
  };
}
#endif // MULTIVERSO_HDFS_FILE_SYS_H_