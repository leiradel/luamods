#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "lexer.h"
#include "templ.h"
#include "path.h"
#include "boot_lua.h"

LUAMOD_API int luaopen_ddlt(lua_State* L) {
    static luaL_Reg const functions[] = {
        {"newLexer", l_newLexer},
        {"newTemplate", l_newTemplate},
        {"realpath", l_realpath},
        {"split", l_split},
        {"join", l_join},
        {"scandir", l_scandir},
        {"stat", l_stat},
        {NULL, NULL}
    };

    int const load_res = luaL_loadbufferx(L, boot_lua, sizeof(boot_lua), "boot.lua", "t");

    if (load_res != LUA_OK) {
        return lua_error(L);
    }

    // Call the chunk to get the function, and then call it with the module.
    lua_call(L, 0, 1);
    luaL_newlib(L, functions);
    lua_call(L, 1, 1);
    return 1;
}
