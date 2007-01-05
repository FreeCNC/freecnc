#include <cstdio>
#include <boost/filesystem/operations.hpp>
#include "dirarchive.h"

using std::string;
using std::vector;
using boost::shared_ptr;

namespace fs = boost::filesystem;
    
namespace VFS
{
    //-------------------------------------------------------------------------
    // DirArchiveFile
    //-------------------------------------------------------------------------

    class DirFile : public File
    {
    public:
        DirFile(const fs::path& dir, const string& filename, bool writable);
        ~DirFile();
        
    protected:
        void do_flush();
        int do_read(vector<char>& buf, int count);
        void do_seek(int pos, int orig);
        int do_write(vector<char>& buf);
        
    private:
        FILE* handle;
    };

    //-------------------------------------------------------------------------
    
    DirFile::DirFile(const fs::path& dir, const string& filename, bool writable)
    {
        // Info
        archive_ = dir.native_directory_string();
        name_ = filename;

        eof_ = false;
        pos_ = 0;
        writable_ = writable;
       
        // Open file
        fs::path pth(dir / filename);
        if (fs::exists(pth) && fs::is_directory(pth)) {
            throw FileNotFound("");
        } else {
            handle = fopen(pth.native_directory_string().c_str(), writable ? "wb" : "rb");
            if (!handle) {
                throw FileNotFound(""); // Improve this
            }
        }
        
        // Calculate size
        do_seek(0, 1);
        size_ = ftell(handle);
        do_seek(0, -1);
    }
    
    DirFile::~DirFile()
    {
        if (handle) {
            fclose(handle);
        }
    }
  
    //-------------------------------------------------------------------------
    
    void DirFile::do_flush()
    {
        fflush(handle);
    }
    
    int DirFile::do_read(vector<char>& buf, int count)
    {
        buf.resize(count);
        int bytesread = (int)fread(&buf[0], sizeof(char), buf.size(), handle);
        buf.resize(bytesread);
        eof_ = feof(handle) != 0;
        pos_ = ftell(handle);
        return bytesread;
    }

    void DirFile::do_seek(int offset, int orig)
    {
        int origin;
        switch (orig) {
            case -1: origin = SEEK_SET; break;
            case  0: origin = SEEK_CUR; break;
            case  1: origin = SEEK_END; break;
            default: return;
        }
        fseek(handle, offset, origin);
        eof_ = feof(handle) != 0;
        pos_ = ftell(handle);
    }
    
    int DirFile::do_write(vector<char>& buf)
    {
        int byteswritten = (int)fwrite(&buf[0], sizeof(char), buf.size(), handle);
        eof_ = feof(handle) != 0;
        pos_ = ftell(handle);
        return byteswritten;
    }    

    //-------------------------------------------------------------------------
    // DirArchive
    //-------------------------------------------------------------------------
  
    DirArchive::DirArchive(const fs::path& dir) : dir(dir) {}    
    DirArchive::~DirArchive() {}
    
    string DirArchive::path()
    {
        return dir.native_directory_string();
    }
    
    shared_ptr<File> DirArchive::open(const std::string& filename, bool writable)
    {
        return shared_ptr<File>(new DirFile(dir, filename, writable));
    }
}
