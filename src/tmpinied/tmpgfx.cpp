#include <stdexcept>
#include <string>
#include <boost/lexical_cast.hpp>

#include "SDL.h"

#include "../freecnc/ui/font.h"
#include "tmpgfx.h"
#include "templatedata.h"

using std::runtime_error;
using std::string;
using boost::lexical_cast;

string tername[] = {"LAND", "WATER", "ROAD", "ROCK", "TREE", "WATER (BLOCKED)",
                       "UNDEFINED", "OTHER"};

TmpGFX::TmpGFX()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        throw runtime_error("Could not initialise SDL: " + string(SDL_GetError()));
    }

    SDL_WM_SetCaption("Templates.ini Editor", NULL);
    screen = SDL_SetVideoMode(640, 480, 16, SDL_HWSURFACE|SDL_DOUBLEBUF);
    if (screen == NULL) {
        throw runtime_error("Could not set video mode: " + string(SDL_GetError()));
    }
}

TmpGFX::~TmpGFX()
{
    SDL_Quit();
}

void TmpGFX::draw(TemplateData* data, TemplateImage* marker, Font* fnt, unsigned int curpos)
{
    SDL_Surface* image;
    SDL_Rect dest;
    unsigned int width;
    unsigned int typeimg;
    static unsigned int blackpix = SDL_MapRGB(screen->format,0, 0, 0);

    shared_ptr<TemplateImage> img = data->image();

    dest.x = 0;
    dest.y = 0;

    dest.w = 640;
    dest.h = 480;
    SDL_FillRect(screen, &dest, blackpix);

    width = 0;
    for (unsigned int i = 0; i < data->c_tiles(); ++i) {
        image = img->getImage(i);
        if(image != NULL ) {
            dest.w = image->w;
            dest.h = image->h;
            SDL_BlitSurface(image, NULL, screen, &dest);
            dest.x += 25*data->width()+10;
            SDL_BlitSurface(image, NULL, screen, &dest);
            SDL_FreeSurface(image);
            typeimg = data->type(i);
            if( typeimg > 1 )
                typeimg = 2;
            image = marker->getImage(typeimg);
            SDL_BlitSurface(image, NULL, screen, &dest);
            SDL_FreeSurface(image);
            dest.x -= 25*data->width()+10;
            if( curpos == i ) {
                image = marker->getImage(3);
                SDL_BlitSurface(image, NULL, screen, &dest);
                SDL_FreeSurface(image);
            }
        }
        dest.x += 25;
        width++;
        if( width == data->width() ) {
            dest.y += 25;
            dest.x = 0;
            width = 0;
        }
    }

    dest.y += 10;

    {
        string text_message = data->index_name() + ": " + data->name();
        fnt->drawText(text_message.c_str(), screen, 0, dest.y);
        dest.y += fnt->getHeight()+1;

        text_message = "Type is: " + lexical_cast<string>((int)data->type(curpos))
            + " (" + tername[data->type(curpos)] + ")";
        fnt->drawText(text_message, screen, 0, dest.y);
    }

    SDL_Flip(screen);
}
