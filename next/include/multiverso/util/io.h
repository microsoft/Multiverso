#ifndef MULTIVERSO_IO_H_
#define MULTIVERSO_IO_H_

/*!
* \file io.h
* \brief Defines io interface.
*/

#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cassert>

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "multiverso/util/log.h"

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

  // only support file and hdfs
  struct URI
  {
    std::string scheme;
    std::string host;
    std::string name;
    std::string path;

    URI(void) {}
    explicit URI(const char *uri) :URI(std::string(uri)){}
    explicit URI(const std::string& uri)
    {
      std::size_t start = 0, pos = uri.find("://", start);
      if (pos == std::string::npos)
      {
        scheme = "file";
      }
      else
      {
        scheme = uri.substr(0, pos);
        start = pos + 3;
      }

      pos = uri.find("/", start);
      if (pos == std::string::npos)
      {
        if (scheme == "hdfs")
          host = "default";
        else
          host = "";
      }
      else
      {
        host = uri.substr(start, pos - start);
        start = pos;
      }

      if (scheme == "hdfs" && host.length() == 0)
        host = "default";

      name = uri.substr(start, std::string::npos);
      path = scheme + "://" + host + name;
      //printf("URI:[%s][%s][%s][%s]\n", uri.c_str(), scheme.c_str(), host.c_str(), name.c_str(), path.c_str());
    }
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
    * \param buf pointer to a memory buffe;
    * \param size the size of buf
    */
    virtual size_t Read(void *buf, size_t size) = 0;

    /*!
    * \brief move the position point to seekOrigin + offset
    * \param offset the offset(bytes number) to change the position point
    * \param seekOrigin the reference position
    */
    virtual void Seek(size_t offset, SeekOrigin seekOrigin) = 0;

    virtual bool Good() = 0;

    /*!
    * \brief flush local buffer
    */
    virtual void Flush() = 0;

    virtual ~Stream(void) {};
  };



  enum class FileType : int
  {
    kFile = 0,
    kDirectory = 1
  };

  struct FileInfo
  {
    FileInfo() : size(0), type(FileType::kFile) {}
    URI uri;
    size_t size;
    FileType type;
  };

  class StreamFactory
  {
  public:
    /*!
    * \brief create a Stream
    * \param path the path of the file
    * \param mode "w" - create an empty file to store data;
    *             "a" - open the file to append data to it
    *             "r" - open the file to read
    * \return the Stream which is used to write or read data
    */
    static std::shared_ptr<Stream> GetStream(const URI& uri,
      const char *mode);

    virtual std::shared_ptr<Stream> Open(const URI& uri,
      const char *mode) = 0;

    virtual void Close() = 0;

    virtual ~StreamFactory() {};

  protected:

    static std::map<std::string, std::shared_ptr<StreamFactory> > instances_;

    StreamFactory() {}
  };

  class TextReader
  {
  public:
    TextReader(const URI &uri, size_t buf_size);

    size_t GetLine(std::string &line);

    ~TextReader();
  private:
    size_t LoadBuffer();

    char* buf_;
    size_t pos_, buf_size_, length_;
    std::shared_ptr<Stream> stream_;
  };

}
#endif // MULTIVERSO_IO_H_
