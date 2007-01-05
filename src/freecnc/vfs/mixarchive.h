#ifndef _VFS_MIXARCHIVE_H
#define _VFS_MIXARCHIVE_H

#include <string>
#include <boost/filesystem/path.hpp>
#include <boost/smart_ptr.hpp>

#include "archive.h"

namespace VFS
{    
    class MixArchive : public Archive
    {
    public:
        MixArchive(const boost::filesystem::path& mixfile);
        ~MixArchive();
        
        std::string path();
        boost::shared_ptr<File> open(const std::string& filename, bool writable);

    private:
        boost::filesystem::path mixfile;
    };
}

#endif
