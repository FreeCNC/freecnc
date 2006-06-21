#include <functional>
#include "SDL.h"
#include "message.h"

using std::list;

namespace
{

struct drawMessage : std::unary_function<void, Message>
{
    drawMessage(Font& font, SDL_Surface* textimg, unsigned int& msgy) : font(font), textimg(textimg), msgy(msgy) {}
    void operator()(const Message& msg) {
        font.drawText(msg.getMessage(), textimg, 0, msgy);
        msgy += font.getHeight()+1;
    }
    Font& font;
    SDL_Surface* textimg;
    unsigned int& msgy;
};

}

MessagePool::MessagePool() : updated(false), textimg(0), msgfont("scorefnt.fnt"), width(0)
{
}

MessagePool::~MessagePool()
{
    SDL_FreeSurface(textimg);
}

SDL_Surface *MessagePool::getMessages()
{
    unsigned int curtick = SDL_GetTicks();
    SDL_Rect dest;

    if (!updated) {
        /// @TODO Replace this with STL magic
        for (list<Message>::iterator i = msglist.begin(); i != msglist.end();) {
            if (i->expired(curtick)) {
                updated = true;
                i = msglist.erase(i);
            } else {
                ++i;
            }
        }
    }
    if (!updated) { // Check again now we've possibly changed the value in the above loop
        return textimg;
    }
    updated = false;
    SDL_FreeSurface(textimg);
    textimg = NULL;
    if (msglist.empty()) {
        return textimg;
    }
    textimg = SDL_CreateRGBSurface(SDL_SWSURFACE|SDL_SRCCOLORKEY, width,
            static_cast<int>(msglist.size()*(msgfont.getHeight()+1)), 16, 0, 0, 0, 0);
    dest.x = 0;
    dest.y = 0;
    dest.w = textimg->w;
    dest.h = textimg->h;
    SDL_FillRect(textimg, &dest, 0);
    SDL_SetColorKey(textimg, SDL_SRCCOLORKEY, 0);
    unsigned int msgy = 0;
    drawMessage dm(msgfont, textimg, msgy);
    for_each(msglist.begin(), msglist.end(), dm);
    return textimg;
}

void MessagePool::postMessage(const std::string& msg)
{
    msglist.push_back(Message(msg, SDL_GetTicks()+10000));
    updated = true;
}


void MessagePool::clear()
{
    msglist.clear();
    updated = true;
}

void MessagePool::refresh()
{
    //Reload the font
    msgfont.reload();

    updated = true;
    getMessages(); //Forces the SDL_Surface to reload
}
