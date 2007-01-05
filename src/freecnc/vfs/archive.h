#ifndef _VFS_ARCHIVE_H
#define _VFS_ARCHIVE_H

#include <stdexcept>
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>

#include "file.h"

namespace VFS
{
    class Archive : private boost::noncopyable
    {
    public:
        virtual ~Archive() {}
        
        virtual std::string path() = 0;
        virtual boost::shared_ptr<File> open(const std::string& filename, bool writable) = 0;
    };
    
    struct ArchiveNotWritable : std::runtime_error
    {
        ArchiveNotWritable() : std::runtime_error("") {}
    };
}
#endif
