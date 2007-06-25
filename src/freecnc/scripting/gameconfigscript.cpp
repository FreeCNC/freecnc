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
        {"load_config", &GameConfigScript::load_config},
        {"vfs_add",     &GameConfigScript::vfs_add},
        {NULL, NULL}
    };
    script.register_methods(this, methods);
}

int GameConfigScript::vfs_add(lua_State* L)
{
    enum { ERROR, NONE, ANY, ALL } required_mode = ERROR;

    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "required");

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
            luaL_error(L, "Invalid value for 'required': Must be one of 'all', 'any' or 'none'.");
        }
    }

    bool found = false;

    lua_rawgeti(L, 1, 1);
    for (size_t i = 2; !lua_isnil(L, -1); lua_rawgeti(L, 1, i++)) {
        const char* name = luaL_checkstring(L, -1);
        fs::path filename(current_directory.top() / name);

        if (!game.vfs.add(filename) && required_mode == ALL) {
            luaL_error(L, "Missing required file: '%s'", filename.string().c_str());
        } else if (required_mode == ANY) {
            found = true;
        }
    }

    if (required_mode == ANY && found == false) {
        // TODO: Better wording, maybe keep the filenames around to show?
        luaL_error(L, "None of a selection of files were available");
    }

    return 0;
}

int GameConfigScript::load_config(lua_State* L)
{
    const char* name = luaL_checkstring(L, 1);
    string filename = (current_directory.top() / name).string();
    current_directory.push((current_directory.top() / name).branch_path());

    int ret = luaL_loadfile(script.L, filename.c_str());
    if (ret != 0) {
        const char* errmsg;
        switch (ret) {
            case LUA_ERRFILE:
                errmsg = "Unable to open file '%s'";
                break;
            case LUA_ERRMEM:
                errmsg = "Memory allocation error whilst loading '%s'";
                break;
            case LUA_ERRSYNTAX:
                errmsg = "Syntax error whilst loading '%s'";
                break;
        }
        luaL_error(L, errmsg, filename.c_str());
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
    const char* lua_msg = lua_tostring(script.L, -1);
    if (lua_msg != NULL) {
        msg += lua_msg;
    }
    throw GameConfigScriptError(msg);
}
