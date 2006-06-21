#ifndef _RENDERER_MESSAGE_H
#define _RENDERER_MESSAGE_H

#include <list>
#include "../freecnc.h"
#include "../ui/ui_public.h"

struct SDL_Surface;

/// @TODO Replace this class with a std::pair<string, unsigned int>
class Message
{
public:
    Message(const std::string& msg, unsigned int deltime) : message(msg), deltime(deltime) {}
    const char *getMessage() const { return message.c_str(); }
    bool expired(unsigned int time) const { return time > deltime; }
private:
    std::string message;
    unsigned int deltime;
};

class MessagePool
{
public:
    MessagePool();
    void setWidth(unsigned int width) {this->width = width;}
    unsigned int getWidth() const {return width;}
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
    unsigned int width;
};

#endif
