#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <boost/filesystem/operations.hpp>
#include "mixarchive.h"

using std::ostringstream;
using std::runtime_error;
using std::string;
using std::vector;
using boost::shared_ptr;

namespace fs = boost::filesystem;

namespace VFS
{
    //-------------------------------------------------------------------------
    // MixFile
    //-------------------------------------------------------------------------

    class MixFile : public File
    {
    public:
        MixFile(const fs::path& mixfile, const string& filename, int lower_boundary, int upper_boundary);
        ~MixFile();
        
    protected:
        void do_flush() {}
        int do_read(vector<char>& buf, int count);
        void do_seek(int pos, int orig);
        int do_write(vector<char>& buf) { return 0; }
        
    private:
        FILE* handle;
        int lower_boundary;
        int upper_boundary;
    };
    
    MixFile::MixFile(const fs::path& dir, const string& filename, int lower_boundary, int upper_boundary)
    {
        // Info
        archive_ = dir.native_directory_string();
        name_ = filename;

        eof_ = false;
        pos_ = 0;
        size_ = upper_boundary - lower_boundary;
        writable_ = false;
        
        this->lower_boundary = lower_boundary;
        this->upper_boundary = upper_boundary;
       
        // Open file
        fs::path pth(dir / filename);
        handle = fopen(pth.native_file_string().c_str(), "rb");
        if (!handle) {
            ostringstream temp;
            temp << "fopen failed for '" << name_ << "' in '" << archive_ << "': " << errno << ": " << strerror(errno);
            throw runtime_error(temp.str());
        }
        fseek(handle, lower_boundary, SEEK_SET);
    }
    
    MixFile::~MixFile()
    {
        if (handle) {
            fclose(handle);
        }
    }
    
    //-------------------------------------------------------------------------
    
    int MixFile::do_read(vector<char>& buf, int count)
    {
        // TODO: Test that this actually works 
        count = pos_ + count > upper_boundary ? upper_boundary - count : count;
        buf.resize(count);
        int bytesread = static_cast<int>(fread(&buf[0], sizeof(char), buf.size(), handle));
        buf.resize(bytesread);

        int tell = ftell(handle);
        eof_ = tell > upper_boundary || feof(handle) != 0;
        pos_ = tell - lower_boundary;

        return bytesread;
    }

    void MixFile::do_seek(int offset, int orig)
    {
        int origin, dest;
        switch (orig) {
            case -1:
                origin = SEEK_SET;
                dest = lower_boundary + offset;
                break;
            case 0:
                origin = SEEK_CUR;
                dest = pos_ + offset;
                break;
            case 1:
                origin = SEEK_END;
                dest = upper_boundary + offset;
                break;
            default:
                return;
        }
        if (dest < lower_boundary || dest > upper_boundary) {
            return; 
        }
        fseek(handle, offset, origin);
        eof_ = feof(handle) != 0;
        pos_ = ftell(handle) - lower_boundary;
    }

    //-------------------------------------------------------------------------
    // MixArchive
    //-------------------------------------------------------------------------
  
    MixArchive::MixArchive(const fs::path& mixfile) : mixfile(mixfile)
    {
        // TODO: Build index of mix file.
    }

    MixArchive::~MixArchive()
    {
    }
        
    shared_ptr<File> MixArchive::open(const std::string& filename, bool writable)
    {
        if (writable) {
            return shared_ptr<File>();
        }
        
        // TODO: Locate file in mix index, get upper and lower boundries
        
        return shared_ptr<File>();
    }

    string MixArchive::path()
    {
        return mixfile.native_file_string();
    }
}
