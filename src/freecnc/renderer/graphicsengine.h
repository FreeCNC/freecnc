#ifndef _RENDERER_GRAPHICSENGINE_H
#define _RENDERER_GRAPHICSENGINE_H

#include "SDL.h"
#include "../freecnc.h"

class GraphicsEngine
{
public:
    GraphicsEngine();
    ~GraphicsEngine();
    void setupCurrentGame();
    void renderScene();
    Uint16 getWidth()
    {
        return width;
    }
    Uint16 getHeight()
    {
        return height;
    }
    SDL_Rect *getMapArea()
    {
        return &maparea;
    }
    void drawVQAFrame(SDL_Surface *frame);
    void clearBuffer();
    void clearScreen();
    /*void postMessage(char *msg) {
        messages->postMessage(msg);
    }*/

    void renderLoading(const std::string& buff, SDL_Surface* logo);
    class VideoError {};
private:
    void clipToMaparea(SDL_Rect *dest);
    void clipToMaparea(SDL_Rect *src, SDL_Rect *dest);
    void drawSidebar();
    void drawLine(Sint16 startx, Sint16 starty,
                  Sint16 stopx, Sint16 stopy, Uint16 width, Uint32 colour);
    SDL_Surface* screen;
    SDL_Surface* icon;
    Uint16 width;
    Uint16 height;
    Uint16 tilewidth;
    SDL_Rect maparea;
    // Minimap zoom factor
    struct {
        Uint8 normal;
        Uint8 max;
    } minizoom;
    // Which zoom is currently used?
    Uint8* mz;
    // Used to avoid SDL_MapRGB in the radar render step.
    std::vector<Uint32> playercolours;
};

#endif
