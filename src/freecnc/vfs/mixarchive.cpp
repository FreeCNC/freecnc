#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <boost/filesystem/operations.hpp>

#include "../lib/blowfish.h"
#include "../lib/westwood_key.h"
#include "mixarchive.h"

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

    enum MixArchiveType
    {
        TD_MIXFILE,
        RA_MIXFILE,
    };

    // Red Alert flags
    const unsigned int RA_MIX_CHECKSUM  = 0x00010000;
    const unsigned int RA_MIX_ENCRYPTED = 0x00020000;

    MixArchiveType archive_type(unsigned int first_four_bytes)
    {
        return first_four_bytes == 0 || first_four_bytes == RA_MIX_CHECKSUM || first_four_bytes == RA_MIX_ENCRYPTED || first_four_bytes == (RA_MIX_ENCRYPTED | RA_MIX_CHECKSUM) ? RA_MIXFILE : TD_MIXFILE;
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
        MixFile(const string& archive, const string& name, int lower_boundary, int size);
        ~MixFile();
        
    protected:
        void do_flush() {}
        int do_read(vector<unsigned char>& buf, int count);
        void do_seek(int pos, int orig);
        int do_write(const vector<unsigned char>& buf) { return 0; }
        
    private:
        void update_state();
        FILE* handle;
        int lower_boundary;
    };
    
    MixFile::MixFile(const string& archive, const string& name, int lower_boundary, int size)
    {
        // Info
        archive_ = archive;
        name_ = name;

        eof_ = false;
        pos_ = 0;
        size_ = size;
        writable_ = false;

        this->lower_boundary = lower_boundary;
       
        // Open file
        handle = fopen(archive_.c_str(), "rb");
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
    
    void MixFile::update_state()
    {
        int tell = ftell(handle);
        eof_ = tell >= lower_boundary + size_ || feof(handle) != 0;
        pos_ = tell - lower_boundary;
    }
    
    //-------------------------------------------------------------------------
    
    int MixFile::do_read(vector<unsigned char>& buf, int count)
    {
        if (eof_) {
            return 0;
        }

        count = min(count, size_ - pos_);
        buf.resize(count);
        int bytesread = static_cast<int>(fread(&buf[0], sizeof(char), buf.size(), handle));
        buf.resize(bytesread);

        update_state();
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
                dest = lower_boundary + size_ + offset;
                break;
            default:
                return;
        }
        if (dest < lower_boundary || dest > lower_boundary + size_) {
            return; 
        }

        fseek(handle, offset, origin);
        update_state();
    }

    //-------------------------------------------------------------------------
    // MixArchive
    //-------------------------------------------------------------------------
  
    MixArchive::MixArchive(const fs::path& mixfile, const vector<string>& subarchives)
        : mixfile(mixfile)
    {
        FILE* mix = fopen(path().c_str(), "rb");
        if (!mix) {
            ostringstream temp;
            temp << "MixArchive: '" << path() << "' fopen failed: " << errno << ": " << strerror(errno);
            throw runtime_error(temp.str());
        }
        
        try {
            // Parse main header
            parse_header(mix, 0);
            
            // Load sub archives
            for (vector<string>::const_iterator it = subarchives.begin(); it != subarchives.end(); ++it) {
                if (it->size() > 12) {
                    throw runtime_error("MixArchive: '" + path() + "': Subarchive '" + *it + "' name too long");
                }

                // Lookup file
                unsigned int id = calc_id(*it);
                Index::iterator find_it = index.find(id);
                if (find_it != index.end()) {
                    parse_header(mix, find_it->second.first);
                } else {
                    throw runtime_error("MixArchive: '" + path() + "': Subarchive '" + *it + "' not found");
                }
            }
        } catch (runtime_error&) {
            fclose(mix);
            throw;
        }
        fclose(mix);
    }

    MixArchive::~MixArchive()
    {
    }
    
    //-------------------------------------------------------------------------
    
    void MixArchive::parse_header(FILE* mix, int base_offset)
    {
        fseek(mix, base_offset, SEEK_SET);

        // Determine archive type
        unsigned int type_header = 0;
        if (fread(&type_header, sizeof(unsigned int), 1, mix) != 1) {
            fclose(mix);
            throw runtime_error("MixArchive: '" + path() + "': Invalid archive type");
        }

        fseek(mix, base_offset, SEEK_SET);
        
        if (archive_type(type_header) == TD_MIXFILE) {
            parse_td_header(mix, base_offset);
        } else {
            parse_ra_header(mix, base_offset);
        }  
    }

    void MixArchive::parse_td_header(FILE* mix, int base_offset)
    {
        // Parse header info
        unsigned char header_buf[6];
        if (fread(&header_buf[0], 1, sizeof(header_buf), mix) != sizeof(header_buf)) {
            throw runtime_error("MixArchive: '" + path() + "': Invalid header");
        }
        
        unsigned short index_size = *reinterpret_cast<unsigned short*>(&header_buf[0]) * 12;
        unsigned int data_size = *reinterpret_cast<unsigned int*>(&header_buf[2]);

        // Read file index
        vector<unsigned char> index_buf(index_size);
        if (fread(&index_buf[0], 1, index_buf.size(), mix) != index_buf.size()) {
            throw runtime_error("MixArchive: '" + path() + "': Invalid file index");
        }

        // 6 = index_size + data_size
        build_index(index_buf, base_offset + 6 + index_size);
    }

    void MixArchive::parse_ra_header(FILE* mix, int base_offset)
    {
        unsigned int flags;
        fread(&flags, sizeof(unsigned int), 1, mix);
        
        if (flags & RA_MIX_ENCRYPTED) {
            // Decrypt WS key and prepare blowfish decrypter
            unsigned char westwood_key[80];
            unsigned char blowfish_key[56];
            if (fread(&westwood_key[0], 1, sizeof(westwood_key), mix) != sizeof(westwood_key)) {
                throw runtime_error("MixArchive: '" + path() + "': Invalid key");
            }
            decode_westwood_key(&westwood_key[0], &blowfish_key[0]);
            Blowfish bf(&blowfish_key[0], sizeof(blowfish_key));

            // Parse header info (blowfish wants 64 bit chunks, so we read two extra bytes, used later)
            unsigned char header_buf[8];
            if (fread(&header_buf[0], 1, sizeof(header_buf), mix) != sizeof(header_buf)) {
                throw runtime_error("MixArchive: '" + path() + "': Invalid header");
            }
            bf.decipher(&header_buf[0], &header_buf[0], sizeof(header_buf));
            
            unsigned short index_size = *reinterpret_cast<unsigned short*>(&header_buf[0]) * 12;
            unsigned int data_size = *reinterpret_cast<unsigned int*>(&header_buf[2]);

            // Read file index
            vector<unsigned char> index_buf(index_size + 2);
            if (fread(&index_buf[2], 1, index_size, mix) != index_size) {
                throw runtime_error("MixArchive: '" + path() + "': Invalid file index");
            }

            // First two bytes of first entry comes from the header_buf (see above)
            index_buf[0] = header_buf[6];
            index_buf[1] = header_buf[7];
            
            // Decipher index. The last two bytes are discarded, not sure why
            bf.decipher(&index_buf[2], &index_buf[2], index_size + 5 & ~7);
            index_buf.resize(index_buf.size() - 2);

            // 92 = RA flags + index_size + data_size + westwood_key + 2 unknown bytes
            build_index(index_buf, base_offset + 92 + index_size);
        } else {
            // 4 = RA flags
            parse_td_header(mix, base_offset + 4);
        }
    }
    
    void MixArchive::build_index(const vector<unsigned char>& index_buf, int base_offset)
    {
        // Build index  
        unsigned int id = 0, offset = 0, size = 0;
        for (Index::size_type i = 0; i < index_buf.size(); i+=12) {
            id     = *reinterpret_cast<const unsigned int*>(&index_buf[i]);
            offset = *reinterpret_cast<const unsigned int*>(&index_buf[i+4]) + base_offset;
            size   = *reinterpret_cast<const unsigned int*>(&index_buf[i+8]);

            // 1422054725 = "local mix database.dat" 
            std::pair<Index::iterator, bool> res = index.insert(Index::value_type(id, IndexValue(offset, size)));
            if (!res.second && res.first->first != 1422054725) {
                ostringstream temp;
                temp << "MixArchive: '" << path() << "': Duplicate index '" << id << "'";
                throw runtime_error(temp.str());
            }            
        }
    }

    //-------------------------------------------------------------------------
        
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
            return shared_ptr<File>(new MixFile(path(), filename, it->second.first, it->second.second));
        }

        return shared_ptr<File>();
    }

    string MixArchive::path()
    {
        return mixfile.native_file_string();
    }
}
