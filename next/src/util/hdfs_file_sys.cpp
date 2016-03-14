#include "multiverso/util/hdfs_file_sys.h"

namespace multiverso
{
HDFSStream::HDFSStream(hdfsFS fs, const URI &uri, const char *mode)
{
    using namespace std;
    is_good_ = true;
	fs_ = fs;
    fp_ = nullptr;
	mode_ = std::string(mode);
    int flag = 0;
    if (!strcmp(mode, "r"))
        flag = O_RDONLY;
    else if (!strcmp(mode, "w"))
        flag = O_WRONLY;
    else if (!strcmp(mode, "a"))
        flag = O_WRONLY | O_APPEND;
    else
    {
        Log::Fatal("HDFSStream: unknown flag %s\n", mode);
        is_good_ = false;
        return;
    }

    fp_ = hdfsOpenFile(fs_, uri.name.c_str(), flag,
        0, 0, 0);

    if (fp_ == nullptr)
    {
        is_good_ = false;
        int errsv = errno;
        Log::Error("Failed to open HDFSStream %s, %s\n", uri.name.c_str(), strerror(errsv));
        return;
    }       

    path_ = uri.name;
}

HDFSStream::~HDFSStream(void)
{
	if (mode_ == "a" || mode_ == "w")
		Flush();
    if (hdfsCloseFile(fs_, fp_) == -1)
    {
        is_good_ = false;
        int errsv = errno;
        Log::Error("Failed to close HDFSStream %s, %s\n", path_.c_str(), strerror(errsv));
    }
}

/*!
* \brief write data to a file
* \param buf pointer to a memory buffer
* \param size data size
*/
void HDFSStream::Write(const void *buf, size_t size)
{
    const char *c_buf = reinterpret_cast<const char*>(buf);
    while (size > 0)
    {
      tSize nwrite = hdfsWrite(fs_, fp_, c_buf, size);
      if (nwrite == -1) 
      {
        is_good_ = false;
        int errsv = errno;
        Log::Fatal("Failed to Write data to HDFSStream %s, %s\n", path_.c_str(), strerror(errsv));
      }
      size_t sz = static_cast<size_t>(nwrite);
      c_buf += sz;
      size -= sz;
    }
}

/*!
* \brief read data from Stream
* \param buf pointer to a memory buffer
* \param size the size of buf
*/
size_t HDFSStream::Read(void *buf, size_t size)
{
    char *c_buf = static_cast<char*>(buf);
    size_t i = 0;
    while (i < size)
    {
        size_t nmax = static_cast<size_t>(std::numeric_limits<tSize>::max());
		//Log::Debug("Begin hdfsRead\n");
        tSize ret = hdfsRead(fs_, fp_, c_buf + i, std::min(size - i, nmax));
		//Log::Debug("hdfsRead return %d, i=%d, size=%d\n", ret, i, size);
        if (ret > 0)
        {
            size_t n = static_cast<size_t>(ret);
            i += n;
        }
        else if (ret == 0) 
        {
            break;
        }
        else
        {
            int errsv = errno;
			if (errno == EINTR)
			{
				Log::Info("Failed to Read HDFSStream %s, %s data is temporarily unavailable\n", path_.c_str(), strerror(errsv));
				continue;
			}
            is_good_ = false;
            Log::Fatal("Failed to Read HDFSStream %s, %s\n", path_.c_str(), strerror(errsv));
        }
    }

    return i;
}

/*!
* \brief move the position point to seekOrigin + offset
* \param offset the offset(bytes number) to change the position point
* \param seekOrigin the reference position
*/
void HDFSStream::Seek(size_t offset, SeekOrigin seekOrigin)
{
    tOffset pos;
    if (seekOrigin == SeekOrigin::kBegin)
    {
        pos = static_cast<tOffset>(offset);
    }
    else if (seekOrigin == SeekOrigin::kCurrent)
    {
        pos = hdfsTell(fs_, fp_);
        if (pos == -1)
        {
          is_good_ = false;
          int errsv = errno;
          Log::Fatal("Failed to get Current position of HDFSStream %s: %s\n", path_.c_str(), strerror(errsv));
        }

        pos += static_cast<tOffset>(offset); 
    }
    else
    {
        hdfsFileInfo *info = hdfsGetPathInfo(fs_, path_.c_str());
        if (info == nullptr)
        {
            is_good_ = false;
            int errsv = errno;
            Log::Fatal("Failed to get Current position of HDFSStream %s: %s\n", path_.c_str(), strerror(errsv));
            return;
        }

        pos = info->mSize + static_cast<tOffset>(offset);
        hdfsFreeFileInfo(info, 1);
    }

    if (hdfsSeek(fs_, fp_, pos) != 0)
    {
        is_good_ = false;
        int errsv = errno;
        Log::Error("Failed to Seek HDFSStream %s: %s\n", path_.c_str(), strerror(errsv));
    }
}

bool HDFSStream::Good()
{
    return is_good_;
}

/*!
* \brief flush local buffer
*/
void HDFSStream::Flush()
{
	if (hdfsHSync(fs_, fp_) == -1)
	{
		is_good_ = false;
		int errsv = errno;
		Log::Error("Failed to Flush HDFSStream %s: %s\n", path_.c_str(), strerror(errsv));
	}
}

HDFSStreamFactory::HDFSStreamFactory(const std::string &host)
{
    namenode_ = host;

#ifdef _CONNECT_NEW_INSTANCE_
  fs_ = hdfsConnectNewInstance(namenode_.c_str(), 0);
#else
  fs_ = hdfsConnect(namenode_.c_str(), 0);
#endif

  if (fs_ == NULL)
  {
	  int errsv = errno;
	  Log::Fatal("Failed connect HDFS namenode '%s', %s\n", namenode_.c_str(), strerror(errsv));
  }
}

HDFSStreamFactory::~HDFSStreamFactory(void)
{
    Close();
}

void HDFSStreamFactory::Close(void)
{
	if (fs_ != nullptr && hdfsDisconnect(fs_) != 0)
	{
		int errsv = errno;
		Log::Fatal("HDFSStream.hdfsDisconnect Error: %s\n", strerror(errsv));
	}
	else
		fs_ = nullptr;
}

/*!
* \brief create a Stream
* \param path the path of the file
* \param mode "w" - create an empty file to store data;
*             "a" - open the file to append data to it
*             "r" - open the file to read
* \return the Stream which is used to write or read data
*/
std::shared_ptr<Stream> HDFSStreamFactory::Open(const URI & uri,
    const char *mode)
{
    return std::shared_ptr<Stream>(new HDFSStream(fs_, uri, mode));   
}
}
