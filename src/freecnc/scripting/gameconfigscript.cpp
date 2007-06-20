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
        {"vfs_add",      &GameConfigScript::vfs_add},
        {"add_config",   &GameConfigScript::add_config},
        {NULL, NULL}
    };
    lua_pushstring(script.L, game.config.mod.c_str());
    lua_setglobal(script.L, "mod");
    script.register_methods(this, methods);
}

int GameConfigScript::vfs_add(lua_State* L)
{
    enum {ERROR, NONE, ANY, ALL} required_mode = ERROR;

    if (!lua_istable(L, -1)) {
        game.log << "GameConfigScipt::vfs_add: Not passed a table\n";
        return 0;
    }
    lua_pushstring(L, "required");
    lua_gettable(L, -2);
    const char* required = lua_tostring(L, -1);
    if (required == NULL) {
        required_mode = ALL;
        game.log << "Missing required section, defaulting to ALL\n";
    } else {
        size_t r_len = strlen(required);
        if ((r_len != 3) && (r_len != 4)) {
            game.log << "GameConfigSCript::vfs_add: Syntax error (invalid length for \"required\" value)\n";
            return 0;
        }
        int v = (required[0] | required[1] << 8 | required[2] << 16);
        if (v == 0x6C6C61) {
            required_mode = ALL;
        } else if (v == 0x796E61) {
            required_mode = ANY;
        } else if (v == 0x6E6F6E) {
            required_mode = NONE;
        } else {
            game.log << "GameConfigSCript::vfs_add: Syntax error (invalid \"required\" value)\n";
            return 0;
        }
    }

    bool found = true;

    size_t i = 1;
    lua_rawgeti(L, 1, i);
    while (!lua_isnil(L, -1)) {
        const char* name = lua_tostring(L, -1);

        if (name == NULL) {
            game.log << "GameConfigScript: Null name in manifest" << endl;
            return 0;
        }
        fs::path filename(current_directory.top() / name);
        bool result = game.vfs.add(filename);
        //game.log << "Adding: " << filename << endl;
        if (!result) {
            if (required_mode == ALL) {
                game.log << "Required file " << filename << " was missing!\n";
                string s("Missing required file: ");
                s += filename.string();
                throw GameConfigScriptError(s);
            } else if (required_mode == ANY) {
                found = false;
            }
        }
        ++i;
        lua_rawgeti(L, 1, i);
    }
    if ((required_mode == ANY) && (found == false)) {
        // TODO: This is crappy wording
        game.log << "None of a selection of files were available";
        throw GameConfigScriptError("None of a selection of files were available");
    }
    return 0;
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

