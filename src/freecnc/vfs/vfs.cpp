#include <boost/algorithm/string.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "dirarchive.h"
#include "mixarchive.h"
#include "vfs.h"

using std::string;
using std::vector;
using boost::shared_ptr;
using boost::to_upper;

namespace fs = boost::filesystem;

namespace 
{
    bool check_extension(const fs::path& pth, const string& ext)
    {
        string path_ext(fs::extension(pth));
        to_upper(path_ext);
        return path_ext == ext;
    }

    bool list_files(const fs::path& pth, string ext, vector<fs::path>& results)
    {
        if (!fs::exists(pth)) {
            return false;
        }
        
        to_upper(ext);

        if (!fs::is_directory(pth)) {
            if (check_extension(pth, ext)) {
                results.push_back(pth);
                return true;
            }
            return false;
        }
        
        fs::directory_iterator files(pth), end;
        for (; files != end; ++files) {
            if (!fs::is_directory(*files) && check_extension(*files, ext)) {
                results.push_back(*files);
            }
        }
        return !results.empty();
    }
}

namespace VFS
{   
    VFS::VFS()
    {
    }

    VFS::~VFS()
    {
    }

    bool VFS::add(const string& dir)
    {
        ArchiveVector vec;
        fs::path pth(dir, fs::no_check);

        // Check that the directory exists
        if (!fs::exists(pth) || !fs::is_directory(pth)) {
            return false;
        }
        
        // Add the directory
        vec.push_back(shared_ptr<DirArchive>(new DirArchive(pth)));

        // Add the mix archives
        vector<fs::path> mixfiles;
        if (list_files(pth, ".mix", mixfiles)) {
            for (vector<fs::path>::iterator it = mixfiles.begin(); it != mixfiles.end(); ++it) {
                vec.push_back(shared_ptr<MixArchive>(new MixArchive(*it, std::vector<std::string>())));
            }
        }

        // Add the loaded archives to the VFS manager
        archives.push_back(vec);

        return true;
    }

    void VFS::remove(const string& dir)
    {
        fs::path pth(dir, fs::no_check);
        string native_path = pth.native_directory_string();
        for (ArchiveVectorVector::iterator it = archives.begin(); it != archives.end(); ++it) {
            if ((*it)[0]->path() == native_path) {
                archives.erase(it);
                break;
            }
        }
    }

    void VFS::remove_all()
    {
        archives.clear();
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
        for (ArchiveVectorVector::iterator it = archives.begin(); it != archives.end(); ++it) {
            ArchiveVector subarchives = *it;
            for (ArchiveVector::iterator subit = subarchives.begin(); subit != subarchives.end(); ++subit) {
                shared_ptr<File> file = (*subit)->open(filename, writable);
                if (file) {
                    return file;
                }
            }
        }
        return shared_ptr<File>();
    }
}
