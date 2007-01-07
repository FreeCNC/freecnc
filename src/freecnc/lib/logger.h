#ifndef _LIB_LOGGER_H
#define _LIB_LOGGER_H

class VFile;

class Logger
{
public:
    Logger();
    void gameMsg(const char *txt, ...);
    void renderGameMsg(bool r)
    {
        rendergamemsg = r;
    }
private:
    bool rendergamemsg;
};

#endif

