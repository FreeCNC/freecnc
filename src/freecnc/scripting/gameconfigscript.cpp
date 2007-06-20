#include <boost/algorithm/string.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <fstream>
#include <string>

#include "gameconfigscript.h"
#include "../freecnc.h"

namespace fs = boost::filesystem;

using std::string;
using std::ifstream;

GameConfigScript::GameConfigScript()
{
    LuaScript::Reg<GameConfigScript> methods[] = {
//        {"vfs_add",      &GameConfigScript::vfs_add},
        {"vfs_add_path", &GameConfigScript::vfs_add_path},
        {"add_config",   &GameConfigScript::add_config},
        {NULL, NULL}
    };
    lua_pushstring(script.L, game.config.mod.c_str());
    lua_setglobal(script.L, "mod");
    script.register_methods(this, methods);
}

int GameConfigScript::vfs_add(lua_State* L)
{
    if (!lua_istable(L, 1)) {
        game.log << "GameConfigScipt::vfs_add: Not passed a table\n";
        return 0;
    }
    lua_rawgeti(L, -1, 1);
    const char* required = lua_tostring(L, 1);
    lua_rawgeti(L, -1, 2);
    const char* name = lua_tostring(L, 1);

    fs::path filename(current_directory.top() / name);
    //TODO: Adding individual files, dealing with failure if required etc.
    //bool result = game.vfs.addfile(game.config.basedir + name);
    bool result = 0;
    game.log << "Adding: " << filename.string() << "\n";
    lua_pushboolean(L, result);
    return 1;
}

int GameConfigScript::vfs_add_path(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        game.log << "GameConfigScipt::vfs_add_path: Not passed a string\n";
        return 0;
    }
    const char* name = lua_tostring(L, 1);
    fs::path newdir(current_directory.top() / name);
    bool result = game.vfs.add(newdir);
    //game.log << "Adding: " << newdir << "(result=" << result << ")\n";
    lua_pushboolean(L, result);
    return 1;
}

int GameConfigScript::add_config(lua_State* L)
{
    if (!lua_isstring(L, 1)) {
        game.log << "GameConfigScipt::add_config: Not passed a string\n";
        return 0;
    }
    const char* name = lua_tostring(L, 1);
    string filename = (current_directory.top() / name).string();
    fs::path newdir(current_directory.top() / name);
    newdir = newdir.branch_path();
    //game.log << "Pushing " << newdir << "\n";
    current_directory.push(newdir);
    //game.log << "Parsing: " << filename << "\n";
    luaL_dofile(script.L, filename.c_str());
    current_directory.pop();
    return 0;
}

void GameConfigScript::parse(const string& path)
{
    current_directory.push(game.config.basedir + "/data");
    //game.log << "CD: " << current_directory.top() << " Parsing: " << path << "\n";
    luaL_dofile(script.L, path.c_str());
}

