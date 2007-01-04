#ifndef _VFS_ARCHIVE_H
#define _VFS_ARCHIVE_H
//
// The archive interface, this is the heart of the VFS. One exists for each
// type of archive - DirArchive, MixArchive etc.
//

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

namespace VFS
{
    class Archive : private boost::noncopyable
    {
    public:
        virtual ~Archive() {}
        
        // Archive info
        virtual std::string archive_path() = 0;

        // File management
        virtual int open(const std::string& filename, bool writable) = 0;
        virtual void close(int filenum) = 0;

        // Reading and Writing
        virtual int read(int filenum, std::vector<char>& buf, int count) = 0;
        virtual int read(int filenum, std::vector<char>& buf, char delim) = 0;
        virtual int write(int filenum, std::vector<char>& buf) = 0;
        virtual void flush(int filenum) = 0;

        // File position
        virtual bool eof(int filenum) const = 0;
        virtual void seek_cur(int filenum, int pos) = 0;
        virtual void seek_end(int filenum, int pos) = 0;
        virtual void seek_start(int filenum, int pos) = 0;
        virtual int tell(int filenum) const = 0;

        // File info
        virtual std::string name(int filenum) const = 0;
        virtual int size(int filenum) const = 0;
    };
}

#endif
