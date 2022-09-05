#include <lua.h>
#include <lauxlib.h>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_MT "Buffer"

typedef struct {
    void const* data;
    size_t size;
    size_t position;
    int parent_ref;
}
Buffer;

#define MODE_L   UINT32_C(0x0059701b)
#define MODE_SB  UINT32_C(0x0059797a)
#define MODE_UB  UINT32_C(0x005979bc)
#define MODE_SWL UINT32_C(0x0b88abdb)
#define MODE_SWB UINT32_C(0x0b88abd1)
#define MODE_UWL UINT32_C(0x0b88b45d)
#define MODE_UWB UINT32_C(0x0b88b453)
#define MODE_SDL UINT32_C(0x0b88a968)
#define MODE_SDB UINT32_C(0x0b88a95e)
#define MODE_UDL UINT32_C(0x0b88b1ea)
#define MODE_UDB UINT32_C(0x0b88b1e0)
#define MODE_SQL UINT32_C(0x0b88ab15)
#define MODE_SQB UINT32_C(0x0b88ab0b)
#define MODE_UQL UINT32_C(0x0b88b397)
#define MODE_UQB UINT32_C(0x0b88b38d)
#define MODE_FL  UINT32_C(0x005977d7)
#define MODE_FB  UINT32_C(0x005977cd)
#define MODE_DL  UINT32_C(0x00597795)
#define MODE_DB  UINT32_C(0x0059778b)
#define MODE_SW  UINT32_C(0x0059798f)
#define MODE_UW  UINT32_C(0x005979d1)
#define MODE_SD  UINT32_C(0x0059797c)
#define MODE_UD  UINT32_C(0x005979be)
#define MODE_SQ  UINT32_C(0x00597989)
#define MODE_UQ  UINT32_C(0x005979cb)
#define MODE_F   UINT32_C(0x0002b60b)
#define MODE_D   UINT32_C(0x0002b609)

static int is_le(void) {
    union {
        uint16_t u16;
        uint8_t u8[2];
    }
    test;

    test.u16 = 1;
    return test.u8[0] != 0;
}

static int push(lua_State* L, void const* data, size_t size, int parent_index);

static Buffer* check(lua_State* const L, int const index) {
    return (Buffer*)luaL_checkudata(L, index, BUFFER_MT);
}

static int l_read(lua_State* const L) {
    Buffer* const self = check(L, 1);

    size_t position = self->position;
    size_t const size = self->size;
    uint8_t const* const data = ((uint8_t const*)self->data) + position;

    if (lua_type(L, 2) == LUA_TNUMBER) {
        lua_Integer const length = lua_tointeger(L, 2);

        if (position + length > size) {
invalid_position:
                lua_pushnil(L);
                lua_pushfstring(L, "invalid position: %I", (lua_Integer)position);
                return 2;
        }

        self->position += length;
        lua_pushlstring(L, (char const*)data, length);
        return 1;
    }

    size_t mode_len = 0;
    char const* const mode = luaL_checklstring(L, 2, &mode_len);
    uint32_t mode_hash = 5381;

    for (size_t i = 0; i < mode_len; i++) {
        mode_hash = mode_hash * 33 + (uint8_t)mode[i];
    }

    int const le = is_le();

    switch (mode_hash) {
        case MODE_SW:
            mode_hash = le ? mode_hash = MODE_SWL : MODE_SWB;
            break;

        case MODE_UW:
            mode_hash = le ? mode_hash = MODE_UWL : MODE_UWB;
            break;

        case MODE_SD:
            mode_hash = le ? mode_hash = MODE_SDL : MODE_SDB;
            break;

        case MODE_UD:
            mode_hash = le ? mode_hash = MODE_UDL : MODE_UDB;
            break;

        case MODE_SQ:
            mode_hash = le ? mode_hash = MODE_SQL : MODE_SQB;
            break;

        case MODE_UQ:
            mode_hash = le ? mode_hash = MODE_UQL : MODE_UQB;
            break;

        case MODE_F:
            mode_hash = le ? mode_hash = MODE_FL : MODE_FB;
            break;

        case MODE_D:
            mode_hash = le ? mode_hash = MODE_DL : MODE_DB;
            break;
    }

    uint64_t value = 0;

    switch (mode_hash) {
        case MODE_SB:
        case MODE_UB:
            if (position + 1 > size) {
                goto invalid_position;
            }

            value = (uint64_t)data[0];
            position++;
            break;

        case MODE_SWL:
        case MODE_UWL:
            if (position + 2 > size) {
                goto invalid_position;
            }

            value = (uint64_t)data[0];
            value |= (uint64_t)data[1] << 8;
            position += 2;
            break;

        case MODE_SWB:
        case MODE_UWB:
            if (position + 2 > size) {
                goto invalid_position;
            }

            value = (uint64_t)data[0] << 8;
            value |= (uint64_t)data[1];
            position += 2;
            break;

        case MODE_SDL:
        case MODE_UDL:
        case MODE_FL:
            if (position + 4 > size) {
                goto invalid_position;
            }

            value = (uint64_t)data[0];
            value |= (uint64_t)data[1] << 8;
            value |= (uint64_t)data[2] << 16;
            value |= (uint64_t)data[3] << 24;
            position += 4;
            break;

        case MODE_SDB:
        case MODE_UDB:
        case MODE_FB:
            if (position + 4 > size) {
                goto invalid_position;
            }

            value = (uint64_t)data[0] << 24;
            value |= (uint64_t)data[1] << 16;
            value |= (uint64_t)data[2] << 8;
            value |= (uint64_t)data[3];
            position += 4;
            break;

        case MODE_SQL:
        case MODE_UQL:
        case MODE_DL:
            if (position + 8 > size) {
                goto invalid_position;
            }

            value = (uint64_t)data[0];
            value |= (uint64_t)data[1] << 8;
            value |= (uint64_t)data[2] << 16;
            value |= (uint64_t)data[3] << 24;
            value |= (uint64_t)data[4] << 32;
            value |= (uint64_t)data[5] << 40;
            value |= (uint64_t)data[6] << 48;
            value |= (uint64_t)data[7] << 56;
            position += 8;
            break;

        case MODE_SQB:
        case MODE_UQB:
        case MODE_DB:
            if (position + 8 > size) {
                goto invalid_position;
            }

            value = (uint64_t)data[0] << 56;
            value |= (uint64_t)data[1] << 48;
            value |= (uint64_t)data[2] << 40;
            value |= (uint64_t)data[3] << 32;
            value |= (uint64_t)data[4] << 24;
            value |= (uint64_t)data[5] << 16;
            value |= (uint64_t)data[6] << 8;
            value |= (uint64_t)data[7];
            position += 8;
            break;

        case MODE_L: {
            if (position == size) {
                lua_pushnil(L);
                return 1;
            }

            luaL_Buffer buffer;
            luaL_buffinit(L, &buffer);

            uint8_t const* const data = (uint8_t const*)self->data;

            while (position < size) {
                uint8_t k = data[position++];

                if (k == '\r') {
                    if (position == size || data[position++] != '\n') {
                        // Stray \r in buffer.
                        lua_pushnil(L);
                        lua_pushfstring(L, "invalid end-of-line: \\r");
                        return 2;
                    }

                    // Push line terminated by \r\n.
                    break;
                }
                else if (k == '\n') {
                    // Push line terminated by \n.
                    break;
                }
                else {
                    luaL_addchar(&buffer, k);
                }
            }

            // Push last line without an end-of-line.
            self->position = position;
            luaL_pushresult(&buffer);
            return 1;
        }

        default:
            lua_pushnil(L);
            lua_pushfstring(L, "invalid mode: \"%s\"", mode);
            return 2;
    }

    self->position = position;

    switch (mode_hash) {
        case MODE_FL:
        case MODE_FB: {
            union {
                float f;
                uint32_t u32;
            }
            conv;

            conv.u32 = (uint32_t)value;
            lua_pushnumber(L, conv.f);
            break;
        }

        case MODE_DL:
        case MODE_DB: {
            union {
                double d;
                uint64_t u64;
            }
            conv;

            conv.u64 = value;
            lua_pushnumber(L, conv.d);
            break;
        }

        case MODE_SB:
        case MODE_SWL:
        case MODE_SWB:
        case MODE_SDL:
        case MODE_SDB:
        case MODE_SQL:
        case MODE_SQB:
            lua_pushinteger(L, (int64_t)value);
            break;

        default:
            lua_pushinteger(L, value);
            break;
    }

    return 1;
}

static int l_seek(lua_State* const L) {
    Buffer* const self = check(L, 1);
    lua_Integer const offset = luaL_checkinteger(L, 2);
    char const* const whence = luaL_optstring(L, 3, "set");

    lua_Integer position = 0;

    if (strcmp(whence, "set") == 0) {
        position = offset;
    }
    else if (strcmp(whence, "cur") == 0) {
        position = self->position + offset;
    }
    else if (strcmp(whence, "end") == 0) {
        position = self->size - offset;
    }
    else {
        lua_pushnil(L);
        lua_pushfstring(L, "invalid seek mode: \"%s\"", whence);
        return 2;
    }

    if (position < 0 || position > self->size) {
        lua_pushnil(L);
        lua_pushfstring(L, "invalid seek position: %I", position);
        return 2;
    }

    self->position = (size_t)position;
    lua_pushinteger(L, position);
    return 1;
}

static int l_tell(lua_State* const L) {
    Buffer const* const self = check(L, 1);
    lua_pushinteger(L, self->position);
    return 1;
}

static int l_size(lua_State* const L) {
    Buffer const* const self = check(L, 1);
    lua_pushinteger(L, self->size);
    return 1;
}

static int l_sub(lua_State* const L) {
    Buffer const* const self = check(L, 1);
    lua_Integer const begin = luaL_checkinteger(L, 2);
    lua_Integer const size = luaL_checkinteger(L, 3);

    if (begin < 0 || begin + size >= self->size) {
        lua_pushnil(L);
        lua_pushfstring(L, "invalid limits: %I and %I", begin, size);
        return 2;
    }

    return push(L, ((uint8_t const*)self->data) + begin, size, 1);
}

static int l_tostring(lua_State* const L) {
    Buffer const* const self = (Buffer*)lua_touserdata(L, 1);
    lua_pushlstring(L, (char const*)self->data, self->size);
    return 1;
}

static int l_gc(lua_State* const L) {
    Buffer const* const self = (Buffer*)lua_touserdata(L, 1);

    if (self->parent_ref == LUA_NOREF) {
        free((void*)self->data);
    }
    else {
        luaL_unref(L, LUA_REGISTRYINDEX, self->parent_ref);
    }

    return 0;
}

static int push(lua_State* const L, void const* const data, size_t const length, int const parent_index) {
    Buffer* const self = (Buffer*)lua_newuserdata(L, sizeof(*self));

    self->data = data;
    self->size = length;
    self->position = 0;

    if (parent_index != 0) {
        lua_pushvalue(L, parent_index);
        self->parent_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else {
        self->parent_ref = LUA_NOREF;
    }

    if (luaL_newmetatable(L, BUFFER_MT)) {
        static const luaL_Reg methods[] = {
            {"read", l_read},
            {"seek", l_seek},
            {"tell", l_tell},
            {"size", l_size},
            {"sub",  l_sub},
            {NULL,   NULL}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_tostring);
        lua_setfield(L, -2, "__tostring");

        lua_pushcfunction(L, l_gc);
        lua_setfield(L, -2, "__gc");
    }

    lua_setmetatable(L, -2);
    return 1;
}

static int l_new(lua_State* const L) {
    size_t length;
    char const* const string = luaL_checklstring(L, 1, &length);

    void* const data = malloc(length);

    if (data == NULL) {
        lua_pushnil(L);
        lua_pushliteral(L, "out of memory");
        return 2;
    }

    memcpy(data, string, length);
    return push(L, data, length, 0);
}

LUAMOD_API int luaopen_buffer(lua_State* const L) {
    static const luaL_Reg functions[] = {
        {"new", l_new},
        {NULL,  NULL}
    };

    static struct {char const* const name; char const* const value;} const info[] = {
        {"_COPYRIGHT", "Copyright (c) 2021-2022 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "1.0.0"},
        {"_NAME", "buffer"},
        {"_URL", "https://github.com/leiradel/luamods/buffer"},
        {"_DESCRIPTION", "Creates a read-only binary array from a string, and allows reads of data types from it"}
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
