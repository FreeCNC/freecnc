#include "luascript.h"
namespace {
    struct PushTable
    {
        PushTable(lua_State* L, const char* table) : L(L), table(table), new_table(true)
        {
            lua_getfield(L, LUA_GLOBALSINDEX, table);
            if (lua_istable(L, -1)) {
                new_table = false;
            } else {
                lua_newtable(L);
            }
        }

        ~PushTable() { if (new_table) { lua_setglobal(L, table); } }

        lua_State* L;
        const char* table;
        bool new_table;
    };
}

LuaScript::LuaScript()
{
    L = lua_open();
}

LuaScript::~LuaScript()
{
    lua_close(L);
}

void LuaScript::register_function(const char* name, lua_CFunction func)
{
    lua_register(L, name, func);
}

void LuaScript::register_function(const char* table, const char* name, lua_CFunction func)
{
    PushTable t(L, table);
    lua_pushstring(L, name);
    lua_pushcfunction(L, func);
    lua_settable(L, -3);
}

void LuaScript::register_functions(luaL_Reg* funcs)
{
    for (;funcs->name;++funcs) {
        register_function(funcs->name, funcs->func);
    }
}

void LuaScript::register_functions(const char* table, luaL_Reg* funcs)
{
    PushTable t(L, table);
    for (;funcs->name;++funcs) {
        lua_pushstring(L, funcs->name);
        lua_pushcfunction(L, funcs->func);
        lua_settable(L, -3);
    }
}
