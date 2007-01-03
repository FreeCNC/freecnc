#include <cstdio>
#include <boost/filesystem/operations.hpp>
#include "dirarchive.h"

using std::map;
using std::string;
using std::vector;
using boost::shared_ptr;

namespace fs = boost::filesystem;

typedef VFS::DirArchiveFile DirFile;
typedef shared_ptr<DirFile> DirFilePtr;
typedef map<int, DirFilePtr> DirFileMap;
    
namespace VFS
{
    //-------------------------------------------------------------------------
    // DirArchiveFile helper class
    //-------------------------------------------------------------------------

    struct DirArchiveFile : boost::noncopyable
    {
        FILE* handle;
        string name;
        bool writable;

        DirArchiveFile(const fs::path& dir, const string& name, bool writable)
            : name(name), writable(writable), handle(0)
        {
            fs::path pth(dir / name);
            if (fs::exists(pth) && fs::is_directory(pth)) {
                return;
            } else {
                handle = fopen(pth.native_directory_string().c_str(), writable ? "wb" : "rb");
            }
        }

        ~DirArchiveFile()
        {
            if (handle) {
                fclose(handle);
            }
        }
    };

    //-------------------------------------------------------------------------
    // DirArchive
    //-------------------------------------------------------------------------
  
    DirArchive::DirArchive(const fs::path& dir)
        : dir(dir), filenum(0)
    {
    }
    
    DirArchive::~DirArchive()
    {
    }
    
    string DirArchive::archive_path()
    {
        return dir.native_directory_string();
    }
    
    //-------------------------------------------------------------------------
    
    int DirArchive::open(const std::string& filename, bool writable)
    {
        DirFilePtr file(new DirFile(dir, filename, writable));
        if (file->handle) {
            files.insert(DirFileMap::value_type(filenum, file));
            return filenum++;
        } else {
            return -1;
        }
    }
    
    void DirArchive::close(int filenum)
    {
        files.erase(filenum);
    }

    //-------------------------------------------------------------------------

    int DirArchive::read(int filenum, vector<char>& buf, int count)
    {
        DirFilePtr file = files.find(filenum)->second;
        buf.resize(count);
        int bytesread = (int)fread(&buf[0], sizeof(char), buf.size(), file->handle);
        buf.resize(bytesread);
        return bytesread;
    }
    
    int DirArchive::read(int filenum, vector<char>& buf, char delim)
    {
        DirFilePtr file = files.find(filenum)->second;        
        int totalbytesread = 0, bytesread = 0, i = 0;
        char buffer[1024];
        while (true) {
            bytesread = (int)fread(buffer, sizeof(char), sizeof(buffer), file->handle);
            for (i = 0; i < bytesread; ++i) {
                if (buffer[i] == delim) {
                    buf.reserve(buf.size() + i + 1);
                    buf.insert(buf.end(), &buffer[0], &buffer[i]);
                    fseek(file->handle, -(bytesread - i - 1), SEEK_CUR);
                    return totalbytesread + i + 1;
                }
            }
            totalbytesread += bytesread;
            if (feof(file->handle)) {
                buf.reserve(buf.size() + bytesread);
                buf.insert(buf.end(), &buffer[0], &buffer[bytesread]);
                return totalbytesread;
            }
        }
        return 0;
    }
    
    int DirArchive::write(int filenum, vector<char>& buf)
    {
        DirFilePtr file = files.find(filenum)->second;
        return (int)fwrite(&buf[0], sizeof(char), buf.size(), file->handle);
    }
    
    void DirArchive::flush(int filenum)
    {
        DirFilePtr file = files.find(filenum)->second;
        fflush(file->handle);
    }
    
    //-------------------------------------------------------------------------
       
    bool DirArchive::eof(int filenum) const
    {
        DirFilePtr file = files.find(filenum)->second;
        return feof(file->handle) > 0;
    }

    void DirArchive::seek_cur(int filenum, int offset)
    {
        DirFilePtr file = files.find(filenum)->second;
        fseek(file->handle, offset, SEEK_CUR);
    }
    
    void DirArchive::seek_end(int filenum, int offset)
    {
        DirFilePtr file = files.find(filenum)->second;
        fseek(file->handle, offset, SEEK_END);
    }

    void DirArchive::seek_start(int filenum, int offset)
    {
        DirFilePtr file = files.find(filenum)->second;
        fseek(file->handle, offset, SEEK_SET);
    }
        
    int DirArchive::tell(int filenum) const
    {
        DirFilePtr file = files.find(filenum)->second;
        return ftell(file->handle);
    }

    //-------------------------------------------------------------------------
    
    string DirArchive::name(int filenum) const
    {
        DirFilePtr file = files.find(filenum)->second;
        return file->name;
    }

    int DirArchive::size(int filenum) const
    {
        DirFilePtr file = files.find(filenum)->second;
        int curpos = ftell(file->handle);
        fseek(file->handle, 0, SEEK_END);
        int size = ftell(file->handle);
        fseek(file->handle, curpos, SEEK_SET);
        return size;
    }    
}
