#ifndef _GAMEENGINE_H
#define _GAMEENGINE_H
//
// Defines the GameEngine and it's associated subclasses
//

#include <exception>
#include <fstream>

#include "basictypes.h"

enum GameType
{
    GAME_TD = 1,
    GAME_RA = 2
};

struct GameConfig
{
    string basedir, homedir;

    // Game options
    string map;
    bool fullscreen;
    int width, height;
    int bpp;
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

/*
class InputEngine;
class NetworkClient;
class NetworkServer;
class Renderer;
class SoundEngine;

class GameEngine
{
public:
    GameEngine() : running(false), inited(false) {}
    ~GameEngine() { Shutdown(); }

    // Initializes or reconfigures all subsytems.
    // It uses the settings in config to do so.
    void Init();

    // Shuts down the engine explicitly.
    void Shutdown();

    // The heart of the engine: Basically just does a bit of bookkeeping and
    // calls Update and Render on the current GameMode.
    void MainLoop();

    // Pushes a new mode on top of the current; When it is popped, the previous
    // mode is resumed.
    void PushMode(shared_ptr<GameMode> mode);

    // Pops the current mode
    void PopMode();

    // Returns the current mode
    shared_ptr<GameMode> CurrentMode();

    // The input subsystem, null if in dedicated-server mode
    scoped_ptr<InputEngine> input;

    // The networking subsystem
    scoped_ptr<NetworkClient> network;

    // The server networking subsytem, null if not hosting a server
    scoped_ptr<NetworkServer> serverNetwork;
    
    // The rendering subsystem, null if in dedicated-server mode
    scoped_ptr<Renderer> renderer;

    // The sound subsystem, null if in dedicated-server mode
    scoped_ptr<SoundEngine> sound;

    // Engine configuration. Used by Init.
    Config config;

    // The logger - used like a stream: game.log << "test" << endl;
    Logger log;

    // Virtual Filesystem, used for nearly all file access; Provides archive
    // abstraction.
    VFS vfs;

private:
    // Whether the engine has been initialized
    bool inited;

    // Whether the engine is currently running (in MainLoop)
    bool running;

    // The configuration used for the latest init. Used to determine which
    // settings have changed when re-initing, and to support recovery.
    Config curconfig;

    // The current GameModes.
    std::stack<shared_ptr<GameMode> > modes;
};
*/

#endif
