#include "crc64.h"

#include <lua.h>
#include <lauxlib.h>

#define CRC64_MT "crc64"

static int l_eq(lua_State* const L) {
    uint64_t* const self = luaL_testudata(L, 1, CRC64_MT);
    uint64_t* const other = luaL_testudata(L, 2, CRC64_MT);

    if (self != NULL && other != NULL) {
        lua_pushboolean(L, *self == *other);
    }
    else {
        lua_pushboolean(L, 0);
    }

    return 1;
}

static int l_tostring(lua_State* const L) {
    uint64_t* const self = luaL_checkudata(L, 1, CRC64_MT);

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "0x%016" PRIx64, *self);

    lua_pushstring(L, buffer);
    return 1;
}

static int push_crc(lua_State* const L, uint64_t const crc) {
    size_t length = 0;
    char const* const string = luaL_checkstring(L, 1);

    uint64_t* const self = lua_newuserdata(L, sizeof(*self));
    *self = crc;

    if (luaL_newmetatable(L, CRC64_MT)) {
        lua_pushcfunction(L, l_eq);
        lua_setfield(L, -2, "__eq");

        lua_pushcfunction(L, l_tostring);
        lua_setfield(L, -2, "__tostring");

        lua_pushliteral(L, "crc64");
        lua_setfield(L, -2, "__name");
    }

    lua_setmetatable(L, -2);
    return 1;
}

static int l_create(lua_State* const L) {
    uint64_t const high = luaL_checkinteger(L, 1) & UINT32_C(0xffffffff);
    uint64_t const low = luaL_checkinteger(L, 2) & UINT32_C(0xffffffff);
    return push_crc(L, high << 32 | low);
}

static int l_compute(lua_State* const L) {
    size_t length = 0;
    char const* const string = luaL_checklstring(L, 1, &length);

    uint64_t const crc = crc64((void const*)string, length);
    return push_crc(L, crc);
}

LUAMOD_API int luaopen_crc64(lua_State* L) {
    static luaL_Reg const functions[] = {
        {"create", l_create},
        {"compute", l_compute},
        {NULL,  NULL}
    };

    static struct {char const* const name; char const* const value;} const info[] = {
        {"_COPYRIGHT", "Copyright (c) 2020 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "1.0.0"},
        {"_NAME", "crc64"},
        {"_URL", "https://github.com/leiradel/luamods/crc64"},
        {"_DESCRIPTION", "Computes the CRC-64 (ECMA 182) of a given string"}
    };

    size_t const functions_count = sizeof(functions) / sizeof(functions[0]) - 1;
    size_t const info_count = sizeof(info) / sizeof(info[0]);

    lua_createtable(L, 0, functions_count + info_count);
    luaL_setfuncs(L, functions, 0);

    for (size_t i = 0; i < info_count; i++) {
        lua_pushstring(L, info[i].value);
        lua_setfield(L, -2, info[i].name);
    }

    return 1;
}
