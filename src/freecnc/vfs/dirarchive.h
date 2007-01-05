#ifndef _VFS_DIRARCHIVE_H
#define _VFS_DIRARCHIVE_H

#include <string>
#include <boost/filesystem/path.hpp>
#include <boost/smart_ptr.hpp>

#include "archive.h"

namespace VFS
{    
    class DirArchive : public Archive
    {
    public:
        DirArchive(const boost::filesystem::path& dir);
        ~DirArchive();
        
        std::string path();
        boost::shared_ptr<File> open(const std::string& filename, bool writable);

    private:
        boost::filesystem::path dir;
    };
}

#endif
