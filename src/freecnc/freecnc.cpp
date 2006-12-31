//
// Entry point, engine initialization
//

#include <exception>
#include <iostream>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "SDL.h"

#include "freecnc.h"

using std::cout;
using std::cerr;
using std::exception;

GameEngine game;
Logger* logger;


int main(int argc, char** argv)
{
    try {
        game.startup(argc, argv);
        game.run();
    } catch (GameOptionsUsageMessage& usage) {
        #if _WIN32
        MessageBox(0, usage.what(), "Command line options - FreeCNC", MB_ICONINFORMATION|MB_OK);
        #else
        cout << usage.what() << endl;
        #endif
        return 1;
    } catch (exception& e) {
        #if _WIN32
        MessageBox(0, e.what(), "Fatal error - FreeCNC", MB_ICONERROR|MB_OK);
        #else
        cerr << "Fatal Error: " << e.what() << endl;
        #endif
        return 1;
    }
    return 0;
}
