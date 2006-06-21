#include <stdlib.h>
#include "tmpgfx.h"

char tername[8][16] = {"LAND", "WATER", "ROAD", "ROCK", "TREE", "WATER (BLOCKED)",
                       "UNDEFINED", "OTHER"};

TmpGFX::TmpGFX()
{
    SDL_WM_SetCaption("Templates.ini Editor", NULL);
    screen = SDL_SetVideoMode(640, 480, 16, SDL_HWSURFACE|SDL_DOUBLEBUF);
    if (screen == NULL) {
        fprintf(stderr, "Unable to init videomode: %s\n", SDL_GetError());
        exit(1);
    }
}

TmpGFX::~TmpGFX()
{
}

void TmpGFX::draw(TemplateData *data, TemplateImage *trans, Font *fnt, unsigned int curpos)
{
    SDL_Surface *image;
    SDL_Rect dest;
    unsigned int i, width;
    TemplateImage *img;
    unsigned int typeimg;
    static unsigned int blackpix = SDL_MapRGB(screen->format,0, 0, 0);

    char textmsg[1024];

    img = data->getImage();

    dest.x = 0;
    dest.y = 0;

    dest.w = 640;
    dest.h = 480;
    SDL_FillRect(screen, &dest, blackpix);

    width = 0;
    for( i = 0; i < data->getNumTiles(); i++ ) {
        image = img->getImage(i);
        if(image != NULL ) {
            dest.w = image->w;
            dest.h = image->h;
            SDL_BlitSurface(image, NULL, screen, &dest);
            dest.x += 25*data->getWidth()+10;
            SDL_BlitSurface(image, NULL, screen, &dest);
            SDL_FreeSurface(image);
            typeimg = data->getType(i);
            if( typeimg > 1 )
                typeimg = 2;
            image = trans->getImage(typeimg);
            SDL_BlitSurface(image, NULL, screen, &dest);
            SDL_FreeSurface(image);
            dest.x -= 25*data->getWidth()+10;
            if( curpos == i ) {
                image = trans->getImage(3);
                SDL_BlitSurface(image, NULL, screen, &dest);
                SDL_FreeSurface(image);
            }
        }
        dest.x += 25;
        width++;
        if( width == data->getWidth() ) {
            dest.y += 25;
            dest.x = 0;
            width = 0;
        }
    }

    dest.y += 10;
    sprintf(textmsg, "%s: %s", data->getNumName(), data->getName());
    fnt->drawText(textmsg, screen, 0, dest.y);
    dest.y += fnt->getHeight()+1;
    sprintf(textmsg, "Type is: %d (%s)", data->getType(curpos), tername[data->getType(curpos)]);
    fnt->drawText(textmsg, screen, 0, dest.y);

    SDL_Flip(screen);
    delete img;
}
