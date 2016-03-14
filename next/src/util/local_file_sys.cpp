#include "multiverso/util/local_file_sys.h"
#include <errno.h>
extern "C" {
#include <sys/stat.h>
}
#ifndef _MSC_VER
extern "C" {
#include <sys/types.h>
#include <dirent.h>
}
#else
#include <Windows.h>
#define stat _stat64
#endif

namespace multiverso
{
LocalStream::LocalStream(const URI& uri, const char *mode)
{
    path_ = uri.path;
    fp_ = fopen(uri.path.c_str(), mode);
    if (fp_ == nullptr)
    {
        is_good_ = false;
        Log::Error("Failed to open LocalStream %s\n", uri.path.c_str());
    }
    else
    {
        is_good_ = true;
    }
}

LocalStream::~LocalStream(void)
{
    is_good_ = false;
    if (fp_ != nullptr)
        std::fclose(fp_);
}

/*!
* \brief write data to a file
* \param buf pointer to a memory buffer
* \param size data size
*/
void LocalStream::Write(const void *buf, size_t size)
{
    if (std::fwrite(buf, 1, size, fp_) != size)
    {
        is_good_ = false;
        Log::Error("LocalStream.Write incomplete\n");
    }
}


/*!
* \brief read data from Stream
* \param buf pointer to a memory buffer
* \param size the size of buf
*/
size_t LocalStream::Read(void *buf, size_t size)
{
    return std::fread(buf, 1, size, fp_);
}

/*!
* \brief move the position point to seekOrigin + offset
* \param offset the offset(bytes number) to change the position point
* \param seekOrigin the reference position
*/
void LocalStream::Seek(size_t offset, SeekOrigin seekOrigin)
{
    if (seekOrigin == SeekOrigin::kBegin)
        std::fseek(fp_, static_cast<long>(offset), SEEK_SET);
    else if (seekOrigin == SeekOrigin::kCurrent)
        std::fseek(fp_, static_cast<long>(offset), SEEK_CUR);
    else
        std::fseek(fp_, static_cast<long>(offset), SEEK_END);
}

bool LocalStream::Good()
{
    return is_good_;
}

/*!
* \brief flush local buffer
*/
void LocalStream::Flush()
{
    std::fflush(fp_);
}

LocalStreamFactory::LocalStreamFactory(const std::string& host)
{
    host_ = host;
}

LocalStreamFactory::~LocalStreamFactory()
{
}

/*!
* \brief create a Stream
* \param path the path of the file
* \param mode "w" - create an empty file to store data;
*             "a" - open the file to append data to it
*             "r" - open the file to read
* \return the Stream which is used to write or read data
*/
std::shared_ptr<Stream> LocalStreamFactory::Open(const URI& uri,
    const char *mode)
{
    return std::shared_ptr<Stream>(new LocalStream(uri, mode));
}

void LocalStreamFactory::Close()
{
    ///TODO
}

}