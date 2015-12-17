#include "io.h"
#include "hdfs_file_sys.h"
#include "local_file_sys.h"


namespace multiverso
{
FileSystem *FileSystem::GetInstance(const std::string type,
    const std::string host)
{
    std::string type_tmp = type;
    if (type.length() == 0)
        type_tmp = "file";
  
    if (instances_.find(make_pair(type_tmp, host)) != instances_.end())
        return instances_[make_pair(type_tmp, host)];

    if (type_tmp == std::string("file"))
        return instances_[make_pair(type_tmp, host)] = new LocalFileSystem(host);
    
    if (type_tmp == std::string("hdfs"))
        return instances_[make_pair(type_tmp, host)] = new HDFSFileSystem(host);

    Log::Error("Can not support the FileSystem '%s'\n", type.c_str());   
}

std::map<std::pair<std::string, std::string>, FileSystem *> FileSystem::instances_;

FileSystem::~FileSystem()
{
    for (auto &p : instances_)
        p.second->Close();
    instances_.clear();
}

TextReader::TextReader(Stream *stream, size_t buf_size)
{
	stream_ = stream;
	buf_size_ = buf_size;
	pos_ = length_ = 0;
    buf_ = new char[buf_size_];
    assert(buf_ != nullptr);
}

size_t TextReader::GetLine(char *line)
{
	Log::Debug("TextReader begin read\n");
	size_t ret = 0;
	bool isEnd = false;
	while (true)
	{
		while(pos_ < length_)
		{
			char & c = buf_[pos_++];
			if (c == '\n')
			{
				isEnd = true;
				break;
			}
			else
			{
				line[ret++] = c;
			}
		}

		// has read '\n' or the end of Stream
		if (isEnd || LoadBuffer() == 0)
		{
			break;
		}
	}
	
	//Log::Debug("TextReader read %d bytes\n", ret);
	line[ret] = '\0';
	return ret;
}

size_t TextReader::LoadBuffer()
{
	assert (pos_ == length_);
	pos_ = length_ = 0;
	return length_ = stream_->Read(buf_, buf_size_);
}

TextReader::~TextReader()
{
	delete [] buf_;
}

}
