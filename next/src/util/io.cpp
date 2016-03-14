#include "multiverso/util/io.h"
#include "multiverso/util/hdfs_file_sys.h"
#include "multiverso/util/local_file_sys.h"


namespace multiverso
{

std::shared_ptr<Stream> StreamFactory::GetStream(const URI& uri,
  const char *mode)
{
  std::string addr = uri.scheme + "://" + uri.host;
  if (instances_.find(addr) == instances_.end()) {
    if (uri.scheme == std::string("file"))
      instances_[addr] = std::shared_ptr<StreamFactory>(new LocalStreamFactory(uri.host));
    else if (uri.scheme == std::string("hdfs"))
      instances_[addr] = std::shared_ptr<StreamFactory>(new HDFSStreamFactory(uri.host));
    else Log::Error("Can not support the StreamFactory '%s'\n", uri.scheme.c_str());
  }
  return instances_[addr]->Open(uri, mode);
}

std::map<std::string, std::shared_ptr<StreamFactory> > StreamFactory::instances_;

TextReader::TextReader(const URI& uri, size_t buf_size)
{
	stream_ = StreamFactory::GetStream(uri, "r");
	buf_size_ = buf_size;
  pos_ = length_ = 0;
  buf_ = new char[buf_size_];
  assert(buf_ != nullptr);
}

size_t TextReader::GetLine(std::string &line)
{
	//Log::Debug("Begin TextReader::GetLine\n");
	line.clear();
	bool isEnd = false;
	while (true)
	{
		//Log::Debug("pos_=%d, length_=%d\n", pos_, length_);
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
				line += c;
			}
		}

		//Log::Debug("pos_=%d, length_=%d, isEnd=%d\n", pos_, length_, isEnd);
		// has read '\n' or the end of Stream
		if (isEnd || LoadBuffer() == 0)
		{
			break;
		}
	}

	return line.size();
}

size_t TextReader::LoadBuffer()
{
	assert (pos_ == length_);
	pos_ = length_ = 0;
	return length_ = stream_->Read(buf_, buf_size_ - 1);
}

TextReader::~TextReader()
{
	delete [] buf_;
}

}
