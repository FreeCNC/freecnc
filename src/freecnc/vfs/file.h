#ifndef _VFS_FILE_H
#define _VFS_FILE_H
//
// The File interface provides a thin wrapper over raw archive calls. This is
// what all client code uses, they never touch the archives directly.
//

#include <cassert>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

#include "archive.h"

namespace VFS
{
    class File : public boost::noncopyable
    {
    public:
        File(Archive& arch, int filenum, bool writable)
            : archive(arch), filenum(filenum), iswritable(writable)
        {}

        ~File()
        {
            archive.close(filenum);
        }
        
        //---------------------------------------------------------------------

        // Reads from the file until count or eof has been reached.
        // buf: The buffer to read to.
        // count: Amount of bytes to read. 
        // returns: Amount of bytes read.
        int read(std::vector<char>& buf, int count)
        {
            assert(!iswritable);
            return archive.read(filenum, buf, count);
        }
        
        // Reads from the file until delim or EOF has been reached.
        // buf: The buffer to read to.
        // delim: The delimiting character, it is discarded in the result.
        // returns: Amount of bytes read, including the delimiter.
        int read(std::vector<char>&buf, char delim)
        {
            assert(!iswritable);
            return archive.read(filenum, buf, delim);
        }
        
        // Reads some text form the file.
        // count: Amount of bytes to read.
        std::string read(int count)
        {
            std::vector<char> buf;
            read(buf, count);
            return std::string(buf.begin(), buf.end());
        }
        
        // Reads from the file until delim has been reached
        // delim: The delimiting character, it is discarded in the result.
        std::string read(char delim)
        {
            std::vector<char> buf;
            read(buf, delim);
            return std::string(buf.begin(), buf.end());
        }
        
        // Reads a line from the file, the linebreak is discarded.
        std::string readline()
        {
            return read('\n'); 
        }
        
        //---------------------------------------------------------------------

        // Writes the contents of buf to the file.
        // returns: Amount of bytes written.
        int write(std::vector<char>& buf)
        {
            assert(iswritable);
            return archive.write(filenum, buf);
        }
        
        // Writes a string of text to the file.
        // returns: Amount of bytes written.
        int write(const std::string& str)
        {
           std::vector<char> buf(str.begin(), str.end());
           return write(buf);
        }
        
        // Writes a line of text to the file, appending a linebreak.
        // returns: Amount of bytes written.
        int writeline(const std::string& line)
        {
           return write(line + '\n');
        }

        // Flushes the file
        void flush()
        {
            assert(iswritable);
            archive.flush(filenum);
        }        

        //---------------------------------------------------------------------
        
        // Whether the file is at the of its data stream
        bool eof() const
        {
            return archive.eof(filenum);
        }
        
        // Seeks from current position.
        // offset: Position to seek to from the current position. May be negative.
        void seek_cur(int offset = 0)
        {
            assert(tell() - offset >= 0);
            archive.seek_cur(filenum, offset);
        }
        
        // Seeks from end of the file.
        // offset: Position to seek to from the end of the file. Must be negative.
        void seek_end(int offset = 0)
        {
            assert(offset <= 0);
            archive.seek_end(filenum, offset);
        }

        // Seeks from the beginning of the file.
        // pos: Position to seek to from the start of the file.
        void seek_start(int pos = 0)
        {
            assert(pos >= 0);
            archive.seek_start(filenum, pos);
        }

        // Returns the current position in the file.
        int tell() const
        {
            return archive.tell(filenum);
        }
        
        //---------------------------------------------------------------------
       
        // Returns the name of the file
        std::string name() const
        {
            return archive.name(filenum);
        }

        // Returns the full path of the file, with the archive root included
        std::string fullname() const
        {
            return "[" + archive.archive_path() + " -> " + archive.name(filenum) + "]";
        }         
                
        // Returns size of the file
        int size() const
        {
            return archive.size(filenum);
        }
        
        // Returns whether or not the file is writable
        bool writable() const
        {
            return iswritable;
        }

    private:
        Archive& archive;
        int filenum;
        bool iswritable;
    };
}

#endif
