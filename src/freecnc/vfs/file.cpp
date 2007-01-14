#include <algorithm>
#include <stdexcept>
#include <string>

#include "file.h"

using std::distance;
using std::find;
using std::logic_error;
using std::string;
using std::vector;

namespace VFS
{
    int File::read(vector<unsigned char>& buf, int count)
    {
        if (writable_) { throw logic_error("File not readable"); }

        return do_read(buf, count);
    }

    string File::read(int count)
    {
        if (writable_) { throw logic_error("File not readable"); }

        vector<unsigned char> buf;
        do_read(buf, count);
        return string(buf.begin(), buf.end());
    }
    
    int File::read(vector<unsigned char>& buf, char delim)
    {
        if (writable_) { throw logic_error("File not readable"); }
        
        vector<unsigned char> buffer;
        vector<unsigned char>::iterator it;
        while (!eof_) {
            do_read(buffer, 1024);
            it = find(buffer.begin(), buffer.end(), delim);
            buf.insert(buf.end(), buffer.begin(), it);
            if (it != buffer.end()) { // Found delim
                do_seek(static_cast<int>(-(distance(it, buffer.end()) - 1)), 0);
                break;
            }
        }
        return (int)buf.size();
    }
        
    string File::read(char delim)
    {
        vector<unsigned char> buf;
        read(buf, delim);
        return string(buf.begin(), buf.end());
    }
    
    string File::readline()
    {
        vector<unsigned char> buf;
        if (read(buf, '\n') > 0) {
            vector<unsigned char>::iterator end = buf.end();
            return string(buf.begin(), *buf.rbegin() == '\r' ? --end : end);
        } else {
            return "";
        }
    }
   
    //-------------------------------------------------------------------------
 
    int File::write(const vector<unsigned char>& buf)
    {
        if (!writable_) { throw logic_error("File not writable"); }

        return do_write(buf);
    }

    int File::write(const string& str)
    {
        if (!writable_) { throw logic_error("File not writable"); }

        vector<unsigned char> buf(str.begin(), str.end());
        return do_write(buf);
    }  
    
    int File::writeline(const string& line)
    {
        return write(line + '\n');
    }

    void File::flush()
    {
        if (!writable_) { throw logic_error("File not writable"); }

        do_flush();
    }     
}
