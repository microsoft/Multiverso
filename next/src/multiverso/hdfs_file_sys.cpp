#include "hdfs_file_sys.h"

namespace multiverso
{
HDFSStream::HDFSStream(hdfsFS fs, hdfsFile fp, std::string path)
{
    fs_ = fs;
    fp_ = fp;
    path_ = path;
}

HDFSStream::~HDFSStream(void)
{
    Flush();
    if (hdfsCloseFile(fs_, fp_) != -1)
    {
        int errsv = errno;
        Log::Error("Failed to close HDFSStream %s\n", strerror(errsv));
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
    while (size != 0)
    {
      tSize nwrite = hdfsWrite(fs_, fp_, c_buf, size);
      if (nwrite == -1) 
      {
        int errsv = errno;
        Log::Fatal("Failed to Write data to HDFSStream %s\n", strerror(errsv));
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
        tSize ret = hdfsRead(fs_, fp_, c_buf + i, std::min(size - i, nmax));
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
            if (errno == EINTR) continue;
            Log::Fatal("Failed to Read HDFSStream %s\n", strerror(errsv));
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
          int errsv = errno;
          Log::Fatal("Failed to get Current position of HDFSStream: %s\n", strerror(errsv));
        }

        pos += static_cast<tOffset>(offset); 
    }
    else
    {
        hdfsFileInfo *info = hdfsGetPathInfo(fs_, path_.c_str());
        if (info == nullptr)
        {
            int errsv = errno;
            Log::Fatal("Failed to get Current position of HDFSStream: %s\n", strerror(errsv));
        }

        pos = info->mSize + static_cast<tOffset>(offset);
        hdfsFreeFileInfo(info, 1);
    }

    if (hdfsSeek(fs_, fp_, pos) != 0)
    {
        int errsv = errno;
        Log::Error("Failed to Seek HDFSStream: %s\n", strerror(errsv));
    }
}

/*!
* \brief flush local buffer
*/
void HDFSStream::Flush()
{
    if (hdfsHSync(fs_, fp_) == -1)
    {
        int errsv = errno;
        Log::Error("Failed to Flush HDFSStream: %s\n", strerror(errsv));
    }
}

HDFSFileSystem::HDFSFileSystem(const std::string host)
{
    if (host.length() == 0)
        namenode_ = "default";
    else
        namenode_ = host;

  fs_ = hdfsConnect(namenode_.c_str(), 0);
  if (fs_ == NULL)
    Log::Fatal("Failed connect HDFS namenode '%s'\n", namenode_.c_str());
}

HDFSFileSystem::~HDFSFileSystem(void)
{

}

void HDFSFileSystem::Close(void)
{
    if (fs_ != nullptr && hdfsDisconnect(fs_) != 0)
    {
        int errsv = errno;
        Log::Fatal("HDFSStream.hdfsDisconnect Error: %s\n", strerror(errsv));
    }
}

/*!
* \brief create a Stream
* \param path the path of the file
* \param mode "w" - create an empty file to store data;
*             "a" - open the file to append data to it
*             "r" - open the file to read
* \return the Stream which is used to write or read data
*/
Stream *HDFSFileSystem::Open(const std::string path,
    const char *mode)
{
    using namespace std;
    int flag = 0;
    if (!strcmp(mode, "r"))
        flag = O_RDONLY;
    else if (!strcmp(mode, "w"))
        flag = O_WRONLY;
    else if (!strcmp(mode, "a"))
        flag = O_WRONLY | O_APPEND;
    else 
        Log::Fatal("HDFSStream: unknown flag %s\n", mode);
    
    hdfsFile fp = hdfsOpenFile(fs_, path.c_str(), flag,
        0, 0, 0);

    if (fp != nullptr)
    {
        return new HDFSStream(fs_, fp, path);
    }
    else
    {
        int errsv = errno;
        Log::Error("Failed to open HDFSStream %s, %s\n", path.c_str(), strerror(errsv));
        return nullptr;
    }       
}

void HDFSFileSystem::CreateDirectory(const std::string path)
{
    if (hdfsCreateDirectory(fs_, path.c_str()) != 0)
    {
        int errsv = errno;
        Log::Error("Failed to CreateDirectory %s, %s\n", path.c_str(), strerror(errsv));
    }
}

/*!
* \brief check if the path exists
* \param path the file or directory
*/
bool HDFSFileSystem::Exists(const std::string path)
{
    return hdfsExists(fs_, path.c_str()) != -1;
}

/*!
* \brief delete the file or directory
* \param path the file or directory
*/
void HDFSFileSystem::Delete(const std::string path)
{
    if (hdfsDelete(fs_, path.c_str(), 1) != 0)
    {
        int errsv = errno;
        Log::Error("Failed to delete %s, %s\n", path.c_str(), strerror(errsv));
    }
}

FileInfo *ConvertFileInfo(const std::string &path, const hdfsFileInfo &info)
{
    FileInfo *ret = new FileInfo();
    ret->path = path;
    ret->size = info.mSize;
    switch (info.mKind)
    {
        case 'D': ret->type = FileType::kDirectory; break;
        case 'F': ret->type = FileType::kFile; break;
        default: Log::Fatal("unknown hdfs file type %c\n", info.mKind);
    }
    return ret;
}

FileInfo* HDFSFileSystem::GetPathInfo(const std::string path)
{
    hdfsFileInfo *info = hdfsGetPathInfo(fs_, path.c_str());
    if (info == nullptr)
    {
        Log::Error("%s does not exist\n", path.c_str());
        return nullptr;
    }

    FileInfo *ret = ConvertFileInfo(path, *info);
    hdfsFreeFileInfo(info, 1);
    return ret;
}

void HDFSFileSystem::Rename(const std::string old_path,
    const std::string new_path)
{
    if (hdfsRename(fs_, old_path.c_str(), new_path.c_str()) != 0)
    {
        int errsv = errno;
        Log::Error("Failed to rename %s to %s, %s\n",
            old_path.c_str(), new_path.c_str(), strerror(errsv));
    }
}

void HDFSFileSystem::Copy(const std::string src, const std::string dst)
{
    if (hdfsCopy(fs_, src.c_str(), fs_, dst.c_str()) != 0)
    {
        int errsv = errno;
        Log::Error("Failed to copy %s to %s, %s\n",
            src.c_str(), dst.c_str(), strerror(errsv));
    }
}

void HDFSFileSystem::ListDirectory(const std::string path,
    std::vector<FileInfo*> &out_list)
{
    int nentry;
    hdfsFileInfo *files = hdfsListDirectory(fs_, path.c_str(), &nentry);
    if (files == nullptr)
    {
        Log::Error("Failed to ListDirectory %s\n", path.c_str());
        return;
    }

    out_list.clear();
    for (int i = 0; i < nentry; ++i)
    {
        std::string tmp(files[i].mName);
        size_t pos = tmp.find('/', 7);
        Log::Debug("ListDirectory file_name=%s, %d\n", tmp.c_str(), pos);
        assert(pos >= 0);
        out_list.push_back(ConvertFileInfo(tmp.substr(pos, std::string::npos), files[i]));
    }
    hdfsFreeFileInfo(files, nentry);
}
}
