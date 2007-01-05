#ifndef _VFS_VFS_H
#define _VFS_VFS_H
//
// The VFS manager. Contains all logic for loading archives, and delegating to
// the right archive when opening files.
//

#include <stdexcept>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>

#include "file.h"

namespace VFS
{
    class Archive;

    // Find better names for these
    typedef std::vector<boost::shared_ptr<Archive> > ArchiveVector; 
    typedef std::vector<ArchiveVector> ArchiveVectorVector;

    class VFS : private boost::noncopyable
    {
    public:
        VFS();
        ~VFS();

        // Adds a directory to the VFS. Directories are searched in the order
        // they are added.
        // All archives present in the root of this path will also be added.
        // Files in the filesystem always takes priority over files in archives.
        bool add(const std::string& dir);

        // Removes a directory from the search paths.
        void remove(const std::string& dir);

        // Removes all directories.
        void remove_all();

        // Opens a file for reading.
        boost::shared_ptr<File> open(const std::string& filename);

        // Opens a file for writing.
        boost::shared_ptr<File> open_write(const std::string& filename);

    private:
        boost::shared_ptr<File> open(const std::string& filename, bool writable);

        // Contains the list of archives; The first archive of each group is the
        // directory that was added, and the remaining members are archives located
        // in that directory.
        ArchiveVectorVector archives;
    };

    struct DirNotFound : std::runtime_error
    {
        DirNotFound(const std::string& message) : std::runtime_error(message) {}
    };
}

#endif
