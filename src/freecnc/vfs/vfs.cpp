#include <boost/algorithm/string.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/operations.hpp>

#include "dirarchive.h"
#include "mixarchive.h"
#include "vfs.h"

using std::string;
using std::vector;
using boost::shared_ptr;
using boost::to_upper;

namespace fs = boost::filesystem;

namespace VFS
{
    VFS::VFS()
    {
    }

    VFS::~VFS()
    {
    }

    bool VFS::add(const fs::path& pth)
    {
        if (!fs::exists(pth)) {
            return false;
        }

        if (fs::is_directory(pth)) {
            archives.push_back(shared_ptr<DirArchive>(new DirArchive(pth)));
            return true;
        } else if (fs::is_regular(pth)) {
            archives.push_back(shared_ptr<MixArchive>(new MixArchive(pth, vector<string>())));
            return true;
        }
        
        return false;
    }

    void VFS::remove_all()
    {
        ArchiveVector().swap(archives);
    }

    shared_ptr<File> VFS::open(const string& filename)
    {
        return open(filename, false);
    }

    shared_ptr<File> VFS::open_write(const string& filename)
    {
        return open(filename, true);
    }

    shared_ptr<File> VFS::open(const string& filename, bool writable)
    {
        for (ArchiveVector::iterator it = archives.begin(); it != archives.end(); ++it) {
            shared_ptr<File> file = (*it)->open(filename, writable);
            if (file) {
                return file;
            }
        }
        return shared_ptr<File>();
    }
}
