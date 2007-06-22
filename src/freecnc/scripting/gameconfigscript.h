#ifndef _LUA_GAMECONFIG_H
#define _LUA_GAMECONFIG_H

#include <stdexcept>
#include <string>
#include <stack>
#include <boost/filesystem/path.hpp>
#include "luascript.h"

class GameConfigScript
{
public:
    GameConfigScript();

    void parse(const std::string& path);

    static LuaScript::Reg<GameConfigScript> lua_methods[];
private:
    LuaScript script;
    std::stack<boost::filesystem::path> current_directory;

    void handle_error();

    int load_config(lua_State* L);
    int vfs_add(lua_State* L);
};


struct GameConfigScriptError : public std::runtime_error
{
    GameConfigScriptError(const std::string& msg) : std::runtime_error(msg) {}
};

#endif
