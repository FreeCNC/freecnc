#include <boost/algorithm/string.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <cassert>
#include <string>

#include "gameconfigscript.h"
#include "../freecnc.h"

namespace fs = boost::filesystem;

using std::string;

GameConfigScript::GameConfigScript()
{
    LuaScript::Reg<GameConfigScript> methods[] = {
        {"vfs_add",      &GameConfigScript::vfs_add},
        {"add_config",   &GameConfigScript::add_config},
        {NULL, NULL}
    };
    lua_pushstring(script.L, game.config.mod.c_str());
    lua_setglobal(script.L, "gamemod");
    script.register_methods(this, methods);
}

int GameConfigScript::vfs_add(lua_State* L)
{
    enum {ERROR, NONE, ANY, ALL} required_mode = ERROR;

    if (!lua_istable(L, -1)) {
        lua_pushstring(L, "vfs_add expects to be passed a table");
        lua_error(L);
    }
    lua_pushstring(L, "required");
    lua_gettable(L, -2);
    const char* required = lua_tostring(L, -1);
    if (required == NULL) {
        required_mode = ALL;
    } else {
        string req(required);
        if (req == "all") {
            required_mode = ALL;
        } else if (req == "any") {
            required_mode = ANY;
        } else if (req == "none") {
            required_mode = NONE;
        } else {
            lua_pushstring(L, "Invalid value provided to \"required\"");
            lua_error(L);
        }
    }

    bool found = false;

    lua_rawgeti(L, 1, 1);
    for (size_t i = 2; !lua_isnil(L, -1); lua_rawgeti(L, 1, i++)) {
        const char* name = luaL_checkstring(L, -1);
        fs::path filename(current_directory.top() / name);

        if (!game.vfs.add(filename) && required_mode == ALL) {
            string s("Missing required file: ");
            s += filename.string();
            lua_pushstring(L, s.c_str());
            lua_error(L);
        } else if (required_mode == ANY) {
            found = true;
        }
    }

    if ((required_mode == ANY) && (found == false)) {
        // TODO: Better wording, maybe keep the filenames around to show?
        lua_pushstring(L, "None of a selection of files were available");
        lua_error(L);
    }
    return 0;
}

int GameConfigScript::add_config(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    string filename = (current_directory.top() / name).string();
    fs::path newdir(current_directory.top() / name);
    newdir = newdir.branch_path();
    //game.log << "Pushing " << newdir << "\n";
    current_directory.push(newdir);
    //game.log << "Parsing: " << filename << "\n";
    int ret = luaL_loadfile(script.L, filename.c_str());
    if (ret != 0) {
        string msg;
        switch(ret) {
        case LUA_ERRFILE:
            msg += "Unable to open file ";
            break;
        case LUA_ERRMEM:
            msg += "Memory allocation error whilst loading ";
            break;
        case LUA_ERRSYNTAX:
            msg += "Syntax error whilst loading ";
            break;
        }
        msg += "\"" + filename + "\"";
        lua_pushstring(script.L, msg.c_str());
        lua_error(L);
    }
    lua_call(script.L, 0, 0);

    current_directory.pop();
    return 0;
}

void GameConfigScript::parse(const string& path)
{
    current_directory.push(game.config.basedir + "/data");
    //game.log << "CD: " << current_directory.top() << " Parsing: " << path << "\n";
    if (luaL_loadfile(script.L, path.c_str()) != 0) {
        handle_error();
    }
    if (lua_pcall(script.L, 0, 0, 0) != 0) {
        handle_error();
    }
}

void GameConfigScript::handle_error()
{
    string msg("GameConfigScript: Lua error: ");
    const char* lua_msg = lua_tostring(script.L, 1);
    if (lua_msg != NULL) {
        msg += lua_msg;
    }
    throw GameConfigScriptError(msg);
}
