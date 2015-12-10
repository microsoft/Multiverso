#include "io.h"
#include "hdfs_file_sys.h"
#include "local_file_sys.h"


namespace multiverso
{
FileSystem *FileSystem::GetInstance(const std::string type,
    const std::string host)
{
    if (type == std::string("file") || type.length() == 0)
        return new LocalFileSystem(host);
    
    if (type == std::string("hdfs"))
        return new HDFSFileSystem(host);

    Log::Error("Can not support the FileSystem '%s'\n", type.c_str());   
}
}