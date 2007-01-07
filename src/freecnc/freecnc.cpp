//
// Entry point, engine initialization
//
#include <exception>
#include "SDL.h"
#include "freecnc.h"

#if _WIN32
#include "win32/text_window.h"
#else
#include <iostream>
#endif

using std::exception;

GameEngine game;
Logger* logger;


int main(int argc, char** argv)
{
    try {
        game.startup(argc, argv);
        game.run();
    } catch (GameOptionsUsageMessage& e) {
        #if _WIN32
        Win32::text_window("Command line options", e.what());
        #else
        std::cout << e.what() << endl;
        #endif
        return 1;
    } catch (std::exception& e) {
        #if _WIN32
        Win32::text_window("Fatal Error", e.what());
        #else
        std::cerr << "Fatal Error: " << e.what() << endl;
        #endif
        return 1;
    }
    return 0;
}
