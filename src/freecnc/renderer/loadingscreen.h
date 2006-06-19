#ifndef _RENDERER_LOADINGSCREEN_H
#define _RENDERER_LOADINGSCREEN_H

#include "../freecnc.h"

struct SDL_Thread;
struct SDL_mutex;

class CPSImage;

class LoadingScreen {
public:
    LoadingScreen();
    ~LoadingScreen();
    void setCurrentTask(const std::string& task);
    const std::string& getCurrentTask() const {
        return task_;
    }
private:
    // Non-copyable
    LoadingScreen(const LoadingScreen&) {}
    LoadingScreen& operator=(const LoadingScreen&) {return *this;}

    static int runRenderThread(void *inst);
    SDL_Thread *renderThread;
    SDL_mutex *lsmutex;
    CPSImage* logo;
    bool done;
    std::string task_;
};

#endif /* LOADINGSCREEN_H */
