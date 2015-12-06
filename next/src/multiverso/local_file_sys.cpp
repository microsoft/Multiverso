#include "local_file_sys.h"
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
LocalStream::LocalStream(FILE * fp, std::string path)
{
    fp_ = fp;
    path_ = path;
}

LocalStream::~LocalStream()
{
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

/*!
* \brief flush local buffer
*/
void LocalStream::Flush()
{
    std::fflush(fp_);
}

LocalFileSystem::LocalFileSystem(std::string host)
{
    host_ = host;
}

/*!
* \brief create a Stream
* \param path the path of the file
* \param mode "w" - create an empty file to store data;
*             "a" - open the file to append data to it
*             "r" - open the file to read
* \return the Stream which is used to write or read data
*/
Stream * LocalFileSystem::Open(const std::string path,
    const char *mode)
{
    FILE *fp = fopen(path.c_str(), mode);
    if (fp == nullptr)
    {
        Log::Error("Failed to open LocalStream %s\n", path.c_str());
        return nullptr;
    }
    else
        return new LocalStream(fp, path);
}

/*!
* \brief check if the path exists
* \param path the file or directory
*/
bool LocalFileSystem::Exists(const std::string path)
{
    ///TODO
}

/*!
* \brief delete the file or directory
* \param path the file or directory
*/
void LocalFileSystem::Delete(const std::string path)
{
    ///TODO
}

FileInfo *LocalFileSystem::GetPathInfo(const std::string path)
{
    struct stat sb;
    if (stat(path.c_str(), &sb) == -1)
    {
        int errsv = errno;
        Log::Fatal("Failed to GetPathInfo %s: %s\n",
            path.c_str(), strerror(errsv));    
    }

    FileInfo *ret = new FileInfo();
    ret->path = path;
    ret->size = sb.st_size;
    if ((sb.st_mode & S_IFMT) == S_IFDIR)
        ret->type = FileType::kDirectory;
    else
        ret->type = FileType::kFile;

    return ret;
}

void LocalFileSystem::Rename(const std::string old_path, const std::string new_path)
{
    ///TODO
}

void LocalFileSystem::Copy(const std::string src, const std::string dst)
{
    ///TODO
}

void LocalFileSystem::ListDirectory(const std::string path, std::vector<FileInfo*> & out_list)
{
#ifndef _MSC_VER
    DIR *dir = opendir(path.c_str());
    if (dir == NULL)
    {
        int errsv = errno;
        Log::Fatal("Failed to ListDirectory %s, %s\n",
            path.c_str(), strerror(errsv));
    }

    out_list.clear();
    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr)
    {
        if (!strcmp(ent->d_name, ".")) continue;
        if (!strcmp(ent->d_name, "..")) continue;
        std::string pp = path;
        if (pp[pp.length() - 1] != '/') {
            pp += '/';
        }
        pp += ent->d_name;
        out_list.push_back(GetPathInfo(pp));
    }
  closedir(dir);
#else
    WIN32_FIND_DATA fd;
    std::string pattern = path + "/*";
    HANDLE handle = FindFirstFile(pattern.c_str(), &fd);
    if (handle == INVALID_HANDLE_VALUE)
    {
        int errsv = GetLastError();
        Log::Fatal("Failed to ListDirectory %s: %s\n",
            path.c_str(), strerror(errsv));
    }
    
    out_list.clear();
    do
    {
        if (strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, ".."))
        {
            std::string pp = path;
            char clast = pp[pp.length() - 1];
            if (pp == ".")
            {
                pp.name = fd.cFileName;
            } else if (clast != '/' && clast != '\\') {
            pp += '/';
            pp += fd.cFileName;
        }
        out_list.push_back(GetPathInfo(pp));
        }
    } while (FindNextFile(handle, &fd));
    FindClose(handle);
#endif       
}
}