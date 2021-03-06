#include "SDL.h"
#include "SDL_thread.h"
#include "cpsimage.h"
#include "loadingscreen.h"
#include "message.h"

class ImageNotFound;

namespace {
    unsigned int oldwidth;
}

/** @TODO Abstract away the dependencies on video stuff so we can use
 * LoadingScreen as part of the server's console output.
 */
LoadingScreen::LoadingScreen()
{
    done = false;
    lsmutex = SDL_CreateMutex();
    oldwidth = pc::msg->getWidth();
    try {
        logo = new CPSImage("title.cps",1);
        pc::msg->setWidth(logo->getImage()->w / 4);
    } catch (ImageNotFound&) {
        game.log << "Could not load startup graphic" << endl;
        logo = 0;
    }
    renderThread = SDL_CreateThread(LoadingScreen::runRenderThread, this);
}

LoadingScreen::~LoadingScreen()
{
    int stat;
    while(SDL_mutexP(lsmutex)==-1) {
        game.log << "Could not lock mutex" << endl;
    }

    done = true;

    while(SDL_mutexV(lsmutex)==-1) {
        game.log << "Could not unlock mutex" << endl;
    }
    SDL_WaitThread(renderThread, &stat);
    SDL_DestroyMutex(lsmutex);
    delete logo;
    pc::msg->setWidth(oldwidth);
}

int LoadingScreen::runRenderThread(void* inst)
{
    bool isDone = false;
    SDL_Event event;
    LoadingScreen *instance = (LoadingScreen*)inst;

    while( !isDone ) {
        while(SDL_mutexP(instance->lsmutex)==-1) {
            game.log << "Could not lock mutex" << endl;
        }

        //render the frame here
        if (instance->logo == 0) {
            pc::gfxeng->renderLoading(instance->task_, 0);
        } else {
            pc::gfxeng->renderLoading(instance->task_,
                    instance->logo->getImage());
        }
        isDone = instance->done;

        while(SDL_mutexV(instance->lsmutex)==-1) {
            game.log << "Could not unlock mutex" << endl;
        }
        // Limit fps to ~10
        while ( SDL_PollEvent(&event) ) {}
        SDL_Delay(500);
    }
    return 0;
}

void LoadingScreen::setCurrentTask(const std::string& task)
{
    while(SDL_mutexP(lsmutex)==-1) {
        game.log << "Could not lock mutex" << endl;
    }
    task_ = task;
    while(SDL_mutexV(lsmutex)==-1) {
        game.log << "Could not unlock mutex" << endl;
    }
}

