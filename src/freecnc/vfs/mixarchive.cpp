#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <boost/filesystem/operations.hpp>
#include "mixarchive.h"

using std::invalid_argument;
using std::min;
using std::ostringstream;
using std::runtime_error;
using std::string;
using std::vector;
using boost::shared_ptr;

namespace fs = boost::filesystem;

namespace
{
    unsigned int calc_id(const string& filename)
    {
        assert(filename.length() <= 12);

        // Convert filename
        char buffer[13] = { 0 };
        std::transform(filename.begin(), filename.end(), buffer, toupper);
        
        // Calculate id
        unsigned int id = 0;
        for (int i=0; buffer[i] != '\0'; i+=4) {
            id = ((id<<1) | ((id>>31) & 1)) + *reinterpret_cast<unsigned int*>(&buffer[i]);
        }

        return id;
    }

}

namespace VFS
{
    //-------------------------------------------------------------------------
    // MixFile
    //-------------------------------------------------------------------------

    class MixFile : public File
    {
    public:
        MixFile(const fs::path& mixfile, const string& name, int lower_boundary, int size);
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
    
    MixFile::MixFile(const fs::path& mixfile, const string& name, int lower_boundary, int size)
    {
        // Info
        archive_ = mixfile.native_file_string();
        name_ = name;

        eof_ = false;
        pos_ = 0;
        size_ = size;
        writable_ = false;

        this->lower_boundary = lower_boundary;
        this->upper_boundary = lower_boundary + size;
       
        // Open file
        handle = fopen(mixfile.native_file_string().c_str(), "rb");
        if (!handle) {
            ostringstream temp;
            temp << "MixArchive: fopen failed for '" << name_ << "' in '" << archive_ << "': " << errno << ": " << strerror(errno);
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
        if (eof_) {
            return 0;
        }

        count = min(count, size_ - pos_);
        buf.resize(count);
        int bytesread = static_cast<int>(fread(&buf[0], sizeof(char), buf.size(), handle));
        buf.resize(bytesread);

        int tell = ftell(handle);
        eof_ = tell >= upper_boundary || feof(handle) != 0;
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
                dest = lower_boundary + pos_ + offset;
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

        int tell = ftell(handle);
        eof_ = tell >= upper_boundary || feof(handle) != 0;
        pos_ = ftell(handle) - lower_boundary;
    }

    //-------------------------------------------------------------------------
    // MixArchive
    //-------------------------------------------------------------------------
  
    MixArchive::MixArchive(const fs::path& mixfile)
        : mixfile(mixfile)
    {
        FILE* mix = fopen(mixfile.native_file_string().c_str(), "rb");
        if (!mix) {
            ostringstream temp;
            temp << "MixArchive: fopen failed for '" + mixfile.native_file_string() << "': " << errno << ": " << strerror(errno);
            throw runtime_error(temp.str());
        }
        
        // Parse header
        vector<char> header_buf(6);
        if (static_cast<int>(fread(&header_buf[0], sizeof(char), header_buf.size(), mix)) != 6) {
            fclose(mix);
            throw runtime_error("MixArchive: Invalid header in '" + mixfile.native_file_string() + "'");
        }
        
        int file_count = *reinterpret_cast<unsigned short*>(&header_buf[0]);
        int data_size = *reinterpret_cast<unsigned int*>(&header_buf[2]);
        int base_offset = file_count * 12 + 6;
        
        // Parse file index
        vector<unsigned int> index_buf(file_count*3);
        if (static_cast<int>(fread(&index_buf[0], sizeof(unsigned int), file_count*3, mix)) != file_count*3) {
            fclose(mix);
            throw runtime_error("MixArchive: Invalid file index in '" + mixfile.native_file_string() + "'");
        }
        
        fclose(mix);
        
        // Build index
        int real_data_size = 0;
        for (int i = 0; i < index_buf.size(); i+=3) {
            // index_buf[i] = id, index_buf[i+1] = offset, index_buf[i+2] = size
            real_data_size += index_buf[i+2];
            std::pair<Index::iterator, bool> ret = index.insert(Index::value_type(index_buf[i], IndexValue(base_offset + index_buf[i+1], index_buf[i+2])));
            if (!ret.second) {
                ostringstream temp;
                temp << "MixArchive: Duplicate index '" << index_buf[i] << "' found in '" << mixfile.native_file_string() << "'";
                throw runtime_error(temp.str());
            }            
        }
       
        // Validate data size
        if (data_size != real_data_size) {
            throw runtime_error("MixArchive: Invalid data size in '" + mixfile.native_file_string() + "'");
        }
    }

    MixArchive::~MixArchive()
    {
    }
        
    shared_ptr<File> MixArchive::open(const std::string& filename, bool writable)
    {
        if (writable) {
            return shared_ptr<File>();
        }
        
        // Filenames can only be 12 chars in mix files
        if (filename.size() > 12) {
            return shared_ptr<File>();
        }
        
        // Lookup file
        unsigned int id = calc_id(filename);
        Index::iterator it = index.find(id);
        if (it != index.end()) {
            return shared_ptr<File>(new MixFile(mixfile, filename, it->second.first, it->second.second));
        }
                
        return shared_ptr<File>();
    }

    string MixArchive::path()
    {
        return mixfile.native_file_string();
    }
}
