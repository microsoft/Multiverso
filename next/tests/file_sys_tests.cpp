#include <cassert>
#include <cstring>
#include <string>
#include <vector>
#include <cstdio>

#include "local_file_sys.h"
#include "hdfs_file_sys.h"
#include "io.h"

using namespace std;
using namespace multiverso;

void file_sys_test(string testdirpath,
    string testfilepath, string testfilepath2,
    string testfilepath3, string file_sys_type,
    string host)
{
    FileSystem *fs = FileSystem::GetInstance(file_sys_type, host);
	if (fs->Exists(testdirpath))
		fs->Delete(testdirpath);
    assert(fs->Exists(testdirpath) == false);
    assert(fs->Exists(testfilepath) == false);
	
	fs->CreateDirectory(testdirpath);
    assert(fs->Exists(testdirpath) == true);
    Stream *write_stream = fs->Open(testfilepath, "w");

    assert(fs->Exists(testdirpath) == true);
    assert(fs->Exists(testfilepath) == true);

    char buf[10000] = {"abcdefghij"};
	for (int i = 6; i < 10000; ++i)
		buf[i] = 'a';
    write_stream->Write((void*)buf, 10000);
    write_stream->Flush();
	delete write_stream;
	write_stream = nullptr;

    Stream *read_stream = fs->Open(testfilepath, "r");
    assert(read_stream->Read((void*)buf, 3) == 3);
    
	FileInfo * fi = fs->GetPathInfo(testfilepath);
    assert(fi != nullptr);
    assert(fi->path == testfilepath);
    assert(fi->size == 3);
    assert(fi->type == FileType::kFile);
    delete fi;
    fi = nullptr;
   
    assert(fs->Open("/null", "r") == nullptr);

    read_stream->Seek(-3, SeekOrigin::kEnd);
    assert(read_stream->Read((void*)buf, 3) == 3);

    read_stream->Seek(1, SeekOrigin::kBegin);
    buf[0] = 0;
    assert(read_stream->Read(buf, 1) == 1);
    assert(buf[0] == 'b');
    assert(read_stream->Read(buf, 3) == 1);
    assert(buf[0] == 'c');

    read_stream->Seek(-1, SeekOrigin::kCurrent);
    assert(read_stream->Read(buf, 1) == 1);
    assert(buf[0] == 'c');
    read_stream->Seek(-3, SeekOrigin::kCurrent);
    read_stream->Seek(1, SeekOrigin::kCurrent);
    assert(read_stream->Read(buf, 1) == 1);
    assert(buf[0] == 'a');

    read_stream->Seek(0, SeekOrigin::kBegin);
    assert(read_stream->Read(buf, 1) == 1);
    assert(buf[0] == 'a');
    read_stream->Seek(-1, SeekOrigin::kBegin);
    assert(read_stream->Read(buf, 1) == 1);
    assert(buf[0] == 'a');

    read_stream->Seek(0, SeekOrigin::kEnd);
    assert(read_stream->Read(buf, 1) == 0);
    read_stream->Seek(1, SeekOrigin::kBegin);
    assert(read_stream->Read(buf, 1) == 0);

    delete write_stream;
    write_stream = nullptr;

    Stream* a_stream = fs->Open(testfilepath, "a");
    a_stream->Write("d", 1);
    read_stream->Seek(0, SeekOrigin::kBegin);
    assert(read_stream->Read(buf, 20) == 4);
    assert(buf[3] == 'd');


    fi = fs->GetPathInfo(testdirpath);
    assert(fi != nullptr);
    assert(fi->path == testdirpath);
    assert(fi->size == 0);
    assert(fi->type == FileType::kDirectory);
    delete fi;
    fi = nullptr;

    fs->Copy(testfilepath, testfilepath2);
    fs->Rename(testfilepath, testfilepath3);
    vector<FileInfo*> files;
    fs->ListDirectory(testdirpath, files);
    assert(files.size() == 2);
    assert(files[0]->path != files[1]->path);
    assert(files[0]->path == testfilepath2
        || files[1]->path == testfilepath2);
    assert(files[0]->path == testfilepath3
        || files[1]->path == testfilepath3);
    assert(files[0]->size == files[1]->size);
    for (int i = 0; i < files.size(); ++i)
        delete files[i];
    files.clear();

    delete read_stream;
    read_stream = nullptr;
    delete a_stream;
    a_stream = nullptr;
    delete write_stream;
    write_stream = nullptr;
    fs->Delete(testdirpath);
    assert(fs->Exists(testdirpath) == false);
    assert(fs->Exists(testfilepath) == false);
    delete fs;
    fs = nullptr;
}

int main()
{
    file_sys_test("/unittestdir1",
    "/unittestdir1/file1", "/unittestdir1/file2",
    "/unittestdir1/file3", "hdfs",
    "");

	return 0;
    file_sys_test("/unittestdir",
    "/unittestdir/file1", "/unittestdir/file2",
    "/unittestdir/file3", "",
    "");

    file_sys_test("/unittestdir",
    "/unittestdir/file1", "/unittestdir/file2",
    "/unittestdir/file3", "file",
    "");
    return 0;
}
