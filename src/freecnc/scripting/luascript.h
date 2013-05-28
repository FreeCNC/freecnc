#ifndef _LUASCRIPT_H
#define _LUASCRIPT_H

extern "C" {
#include "lualib.h"
#include "lauxlib.h"
}

class LuaScript
{
public:
    template<class T>
    struct Reg
    {
        const char* name;
        int (T::*func)(lua_State*);
    };

    LuaScript();
    ~LuaScript();

    void register_function(const char* name, lua_CFunction func);
    // Register a function and store it inside a table.
    void register_function(const char* table, const char* name, lua_CFunction func);
    // Batch registration of functions into the top level namespace
    void register_functions(luaL_Reg* funcs);
    // Batch registration into a named table.
    void register_functions(const char* table, luaL_Reg* funcs);

    // Batch registration of member functions into a table
    template<class T>
    void register_methods(const char* name, T* obj, Reg<T>* methods);

    // Batch registration of member functions
    template<class T>
    void register_methods(T* obj, Reg<T>* methods);

    // This is public because you'd otherwise have to provide a lot of trivial
    // passthrough functions to wrap the LUA api into this class's public API.
    lua_State* L;
private:
    template<class T>
    void push_member_function(T* obj, int (T::*func)(lua_State*));

    bool start_table(const char* table);
    void end_table();
};

#define LUASCRIPT_IMPL_H
#include "luascript_impl.h"
#undef LUASCRIPT_IMPL_H

#endif
