#ifndef _VFS_MIXARCHIVE_H
#define _VFS_MIXARCHIVE_H

#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <boost/filesystem/path.hpp>
#include <boost/smart_ptr.hpp>

#include "archive.h"

namespace VFS
{    
    class MixArchive : public Archive
    {
        typedef std::pair<int, int> IndexValue;
        typedef std::map<unsigned int, IndexValue> Index;
    public:
        MixArchive(const boost::filesystem::path& mixfile, const std::vector<std::string>& subarchives);
        ~MixArchive();
        
        std::string path();
        boost::shared_ptr<File> open(const std::string& filename, bool writable);

    private:
        boost::filesystem::path mixfile;
        Index index;
        
        void parse_header(FILE* mix, int base_offset);
        void parse_td_header(FILE* mix, int base_offset);
        void parse_ra_header(FILE* mix, int base_offset);
        void build_index(const std::vector<unsigned char>& index_buf, int base_offset);
    };
}

#endif
