#ifndef _RENDERER_MESSAGE_H
#define _RENDERER_MESSAGE_H

#include <list>
#include "../freecnc.h"
#include "../ui/ui_public.h"

struct SDL_Surface;

/// @TODO Replace this class with a std::pair<string, Uint32>
class Message
{
public:
    Message(const std::string& msg, Uint32 deltime) : message(msg), deltime(deltime) {}
    const char *getMessage() const { return message.c_str(); }
    bool expired(Uint32 time) const { return time > deltime; }
private:
    std::string message;
    Uint32 deltime;
};

class MessagePool
{
public:
    MessagePool();
    void setWidth(Uint32 width) {this->width = width;}
    Uint32 getWidth() const {return width;}
    ~MessagePool();
    SDL_Surface *getMessages();
    void postMessage(const std::string& msg);
    void clear();
    void refresh();
private:
    std::list<Message> msglist;
    bool updated;
    SDL_Surface* textimg;
    Font msgfont;
    Uint32 width;
};

#endif
