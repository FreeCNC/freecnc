#ifndef _GAMEENGINE_H
#define _GAMEENGINE_H
//
// Defines the GameEngine and it's associated subclasses
//

#include <exception>
#include <fstream>

#include "basictypes.h"
#include "vfs/vfs.h"

using VFS::File;

enum GameType
{
    GAME_TD = 1,
    GAME_RA = 2
};

struct GameConfig
{
    string basedir, homedir;

    // Game options
    string mod;
    string map;
    bool fullscreen;
    int width, height, bpp;
    GameType gametype;

    // Config only options
    bool play_intro;
    bool scale_movies;
    int scaler_quality;
    int scrollstep, scrolltime, maxscroll;
    int final_delay;
    int buildable_radius;
    double buildable_ratio;

    // Debug flags
    bool nosound;
    bool debug;
};

class GameScreen 
{
public:
    virtual ~GameScreen() {}
    virtual void run() = 0;
};

class GameEngine
{
public:
    GameEngine() {}
    ~GameEngine() { shutdown(); }
   
    void reconfigure();
    void run();
    void startup(int argc, char** argv);
    void shutdown();
    
    void setscreen(GameScreen* newscreen = 0) { screen.reset(newscreen); }
    
    std::ofstream log;
    GameConfig config;
    VFS::VFS vfs;

private:
    scoped_ptr<GameScreen> screen;
    void parse_options(int argc, char** argv);
};

class GameOptionsUsageMessage : public std::exception
{
public:
    GameOptionsUsageMessage(const string& message) : message(message) {}
    ~GameOptionsUsageMessage() throw() {}
    const char* what() const throw() {return message.c_str();}
private:
    string message;
};

#endif
