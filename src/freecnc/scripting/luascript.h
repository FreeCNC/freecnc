#ifndef _LUASCRIPT_H
#define _LUASCRIPT_H

extern "C" {
#include "../../lua/lua.h"
#include "../../lua/lauxlib.h"
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
    void register_function(const char* table, const char* name, lua_CFunction func);
    void register_functions(luaL_Reg* funcs);
    void register_functions(const char* table, luaL_Reg* funcs);

    template<class T>
    void register_methods(const char* name, T* obj, Reg<T>* methods);

    template<class T>
    void register_methods(T* obj, Reg<T>* methods);

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
