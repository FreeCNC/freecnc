#ifndef _VFS_VFS_H
#define _VFS_VFS_H
//
// The VFS manager. Contains all logic for loading archives, and delegating to
// the right archive when opening files.
//

#include <string>
#include <vector>
#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>

#include "file.h"

namespace VFS
{
    class Archive;

    class VFS : private boost::noncopyable
    {
        typedef std::vector<boost::shared_ptr<Archive> > ArchiveVector;
    public:
        VFS();
        ~VFS();

        // Adds a directory or mixfile to the VFS. Directories are searched in
        // the order they are added.
        // Files in the filesystem always takes priority over files in archives.
        bool add(const boost::filesystem::path& pth);

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
        ArchiveVector archives;
    };
}

#endif
