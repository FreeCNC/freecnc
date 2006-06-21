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
    unsigned short getWidth()
    {
        return width;
    }
    unsigned short getHeight()
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
    void drawLine(short startx, short starty,
                  short stopx, short stopy, unsigned short width, unsigned int colour);
    SDL_Surface* screen;
    SDL_Surface* icon;
    unsigned short width;
    unsigned short height;
    unsigned short tilewidth;
    SDL_Rect maparea;
    // Minimap zoom factor
    struct {
        unsigned char normal;
        unsigned char max;
    } minizoom;
    // Which zoom is currently used?
    unsigned char* mz;
    // Used to avoid SDL_MapRGB in the radar render step.
    std::vector<unsigned int> playercolours;
};

#endif
