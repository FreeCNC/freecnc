#include <cstdio>
#include <boost/filesystem/operations.hpp>
#include "mixarchive.h"

using std::map;
using std::string;
using std::vector;
using boost::shared_ptr;

namespace fs = boost::filesystem;

typedef VFS::MixArchiveFile MixFile;
typedef shared_ptr<MixFile> MixFilePtr;
typedef map<int, MixFilePtr> MixFileMap;
    
namespace VFS
{
    //-------------------------------------------------------------------------
    // MixArchiveFile helper class
    //-------------------------------------------------------------------------

    struct MixArchiveFile : boost::noncopyable
    {
        FILE* handle;
        string name;
        int lower_boundry;
        int upper_boundry;
        bool eof;

        MixArchiveFile(const fs::path& mixfile, const string& name, int lower_boundry, int upper_boundry)
            : name(name), lower_boundry(lower_boundry), upper_boundry(upper_boundry), eof(false)
        {
            handle = fopen(mixfile.native_directory_string().c_str(), "rb");
            ::fseek(handle, lower_boundry, SEEK_SET);
        }
        
        ~MixArchiveFile()
        {
            if (handle) {
                fclose(handle);
            }
        }
        
        // Thin wrapper over feof that stays within the boundries
        bool feof()
        {
            return eof || ::feof(handle);
        }
         
        // Thin wrapper over fread that stays within the boundries
        size_t fread(void* dest, size_t element_size, size_t count)
        {
            int pos = ftell(handle);
            int realcount = pos + count > upper_boundry ? upper_boundry - pos : count;
            if (pos + count >= upper_boundry) eof = true;
            return ::fread(dest, element_size, realcount, handle);
        }
        
        // Thin wrapper over fseek that stays within the boundries
        void fseek(long offset, int origin)
        {
            int dest;
            switch (origin) {
                case SEEK_CUR:
                    dest = ftell(handle) + offset; 
                    break;
                case SEEK_END:
                    dest = upper_boundry + offset; 
                    break;
                case SEEK_SET:
                    dest = lower_boundry + offset;
                    break;
                default:
                    return;
            }
            if (dest < lower_boundry || dest > upper_boundry) {
                return; 
            }
            eof = false;
            ::fseek(handle, dest, origin);
        }
    };

    //-------------------------------------------------------------------------
    // MixArchive
    //-------------------------------------------------------------------------
  
    MixArchive::MixArchive(const fs::path& mixfile)
        : curfilenum(0), mixfile(mixfile)
    {
        // TODO: Build the mix file.
    }
    
    MixArchive::~MixArchive()
    {
    }
    
    string MixArchive::archive_path()
    {
        return mixfile.native_directory_string();
    }
    
    //-------------------------------------------------------------------------
    
    int MixArchive::open(const std::string& filename, bool writable)
    {
        if (writable) {
            return -1;
        }
        
        // TODO: Locate file in mix index, get upper and lower boundries
        
        MixFilePtr file(new MixFile(mixfile, filename, 0, 0));
        if (file->handle) {
            files.insert(MixFileMap::value_type(curfilenum, file));
            return curfilenum++;
        } else {
            return -1;
        }
    }
    
    void MixArchive::close(int filenum)
    {
        files.erase(filenum);
    }

    //-------------------------------------------------------------------------

    int MixArchive::read(int filenum, vector<char>& buf, int count)
    {
        MixFilePtr file = files.find(filenum)->second;
        buf.resize(count);
        int bytesread = file->fread(&buf[0], sizeof(char), buf.size());
        buf.resize(bytesread);
        return bytesread;
    }
    
    int MixArchive::read(int filenum, vector<char>& buf, char delim)
    {
        MixFilePtr file = files.find(filenum)->second;        
        int totalbytesread = 0, bytesread = 0, i = 0;
        char buffer[1024];
        while (true) {
            bytesread = file->fread(buffer, sizeof(char), sizeof(buffer));
            for (i = 0; i < bytesread; ++i) {
                if (buffer[i] == delim) {
                    buf.reserve(buf.size() + i + 1);
                    buf.insert(buf.end(), &buffer[0], &buffer[i]);
                    file->fseek(-(bytesread - i - 1), SEEK_CUR);
                    return totalbytesread + i + 1;
                }
            }
            totalbytesread += bytesread;
            if (file->feof()) {
                buf.reserve(buf.size() + bytesread);
                buf.insert(buf.end(), &buffer[0], &buffer[bytesread]);
                return totalbytesread;
            }
        }
        return 0;
    }
    
    int MixArchive::write(int filenum, vector<char>& buf)
    {
        return 0;
    }
    
    void MixArchive::flush(int filenum)
    {
        return;
    }
    
    //-------------------------------------------------------------------------
       
    bool MixArchive::eof(int filenum) const
    {
        MixFilePtr file = files.find(filenum)->second;
        return file->feof();
    }

    void MixArchive::seek_start(int filenum, int offset)
    {
        MixFilePtr file = files.find(filenum)->second;
        file->fseek(offset, SEEK_SET);
    }
    
    void MixArchive::seek_end(int filenum, int offset)
    {
        MixFilePtr file = files.find(filenum)->second;
        file->fseek(offset, SEEK_END);
    }
    
    void MixArchive::seek_cur(int filenum, int offset)
    {
        MixFilePtr file = files.find(filenum)->second;
        file->fseek(offset, SEEK_CUR);
    }
    
    int MixArchive::tell(int filenum) const
    {
        MixFilePtr file = files.find(filenum)->second;
        return ftell(file->handle) - file->lower_boundry;
    }

    //-------------------------------------------------------------------------
    
    string MixArchive::name(int filenum) const
    {
        MixFilePtr file = files.find(filenum)->second;
        return file->name;
    }

    int MixArchive::size(int filenum) const
    {
        MixFilePtr file = files.find(filenum)->second;
        return file->upper_boundry - file->lower_boundry;
    }    
}
