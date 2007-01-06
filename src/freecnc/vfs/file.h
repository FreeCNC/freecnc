#ifndef _VFS_FILE_H
#define _VFS_FILE_H

#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

namespace VFS
{   
    class File : private boost::noncopyable
    {
    public:
        virtual ~File() {}

        // Reads `count' bytes or until EOF from the file into `buf'.
        // Returns bytes read.
        int read(std::vector<char>& buf, int count);

        // Reads `count' bytes or until EOF from the file into a string and
        // returns it.
        std::string read(int count);
        
        // Reads until `delim' or EOF from the file into `buf'.
        // The delimiter is discarded.
        // Returns bytes read, including the delimiter.
        int read(std::vector<char>& buf, char delim);
        
        // Reads until `delim' or EOF from the file into a string and returns it.
        // The delimiter is discarded.
        std::string read(char delim);
        
        // Reads a line from the file and returns it. The linebreak is discarded.
        std::string readline();
        
        //---------------------------------------------------------------------

        // Writes the contents of `buf' to the file.
        // Returns bytes written.
        int write(std::vector<char>& buf);
        
        // Writes `str' to the file.
        // Returns bytes written.
        int write(const std::string& str);
        
        // Writes `line' to the file, appending a linebreak.
        // Returns bytes written.
        int writeline(const std::string& line);

        // Flushes the file
        void flush();

        //---------------------------------------------------------------------
        
        // Seeks from the current position in the file.
        void seek_cur(int offset) { do_seek(offset, 0); }
        
        // Seeks from the end of the file.
        void seek_end(int offset = 0) { do_seek(offset, 1); }
        
        // Seeks form the beginning of the file.
        void seek_start(int offset = 0) { do_seek(offset, -1); }
       
        //---------------------------------------------------------------------
        
        // Name of the archive the file is in.
        std::string archive() const { return archive_; }
        
        // Name of the file.
        std::string name() const { return name_; }

        // Whether the stream is at the end-of-file.
        bool eof() const { return eof_; }
        
        // Current position in the file.
        int pos() const { return pos_; }
        
        // Size of the file in bytes.
        int size() const { return size_; }
        
        // Whether the file is readable or writable.
        bool writable() const { return writable_; }
        
    protected:
        std::string archive_;
        std::string name_;

        bool eof_;
        int pos_;
        int size_;
        bool writable_;

        virtual void do_flush() = 0;
        virtual int do_read(std::vector<char>& buf, int count) = 0;
        virtual void do_seek(int pos, int orig) = 0; // orig = -1 - start, 0 - cur, 1 - end
        virtual int do_write(std::vector<char>& buf) = 0;
    }; 
}

#endif
