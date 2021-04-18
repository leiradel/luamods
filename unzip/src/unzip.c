#include <lua.h>
#include <lauxlib.h>

#include <zconf.h>
#include <unzip.h>

#include <stdlib.h>
#include <string.h>

#define UNZIP_MT "unzFile"

typedef struct {
    zlib_filefunc_def io;
    int object_ref;
    lua_State* L;
    unzFile file;
}
Unzip;

static Unzip* check(lua_State* const L, int const index) {
    return (Unzip*)luaL_checkudata(L, index, UNZIP_MT);
}

static int l_exists(lua_State* const L) {
    Unzip* const self = check(L, 1);
    char const* const path = luaL_checkstring(L, 2);

    int const exists = unzLocateFile(self->file, path, 1) == UNZ_OK;
    lua_pushboolean(L, exists);
    return 1;
}

static void l_push_string_writer(lua_State* const L) {
    int const load_res = luaL_loadstring(L,
        "return {\n"
        "    chunks = {},\n"
        "    write = function(self, chunk)\n"
        "        self.chunks[#self.chunks + 1] = chunk\n"
        "        return self\n"
        "    end,\n"
        "    result = function(self)\n"
        "        return table.concat(self.chunks,'')\n"
        "    end\n"
        "}\n"
    );

    if (load_res != LUA_OK) {
        lua_error(L);
        return;
    }

    lua_call(L, 0, 1);
}

static int l_read(lua_State* const L) {
    Unzip* const self = check(L, 1);
    char const* const path = luaL_checkstring(L, 2);
    int const string_writer = lua_isnoneornil(L, 3);

    if (string_writer) {
        lua_settop(L, 2);
        l_push_string_writer(L);
    }
    else {
        lua_settop(L, 3);
    }

    if (unzLocateFile(self->file, path, 1) != UNZ_OK) {
        lua_pushnil(L);
        lua_pushfstring(L, "could not find file \"%s\" in archive", path);
        return 2;
    }

    if (unzOpenCurrentFile(self->file) != UNZ_OK) {
        lua_pushnil(L);
        lua_pushfstring(L, "error opening file \"%s\"", path);
        return 2;
    }

    lua_getfield(L, 3, "write");

    char buffer[256];

    for (;;) {
        int const num_read = unzReadCurrentFile(self->file, buffer, sizeof(buffer));

        if (num_read == 0) {
            break;
        }
        else if (num_read < 0) {
            unzCloseCurrentFile(self->file);
            lua_pushnil(L);
            lua_pushfstring(L, "error reading from file \"%s\" in archive", path);
            return 2;
        }

        lua_pushvalue(L, 4);
        lua_pushvalue(L, 3);
        lua_pushlstring(L, buffer, (size_t)num_read);

        lua_call(L, 2, 2);

        if (lua_isnil(L, -2)) {
            return 2;
        }

        lua_pop(L, 2);
    }

    if (string_writer) {
        lua_pushvalue(L, 3);
        lua_getfield(L, -1, "result");
        lua_insert(L, -2);
        lua_call(L, 1, 1);
    }
    else {
        lua_pushvalue(L, 1);
    }

    return 1;
}

static int l_gc(lua_State* const L) {
    Unzip* const self = (Unzip*)lua_touserdata(L, 1);
    unzClose(self->file);
    luaL_unref(L, LUA_REGISTRYINDEX, self->object_ref);
    return 0;
}

static voidpf zip_open(voidpf const opaque, char const* const filename, int const mode) {
    (void)filename;

    if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) != ZLIB_FILEFUNC_MODE_READ) {
        return NULL;
    }

    return opaque;
}

static uLong zip_read(voidpf const opaque, voidpf const stream, void* const buf, uLong const size) {
    (void)opaque;

    Unzip* const self = (Unzip*)stream;

    lua_rawgeti(self->L, LUA_REGISTRYINDEX, self->object_ref);
    lua_getfield(self->L, -1, "read");
    lua_insert(self->L, -2);

    lua_pushinteger(self->L, (lua_Integer)size);
    // Returns string with contents, offset into string (0-based), number of characters available
    lua_call(self->L, 2, 3);

    size_t length;
    char const* const string = lua_tolstring(self->L, -3, &length);
    lua_Integer const ilen = (lua_Integer)length;

    lua_Integer offset = luaL_optinteger(self->L, -2, 0);
    lua_Integer available = luaL_optinteger(self->L, -1, ilen);

    if (offset < 0) {
        available += offset;
        offset = 0;
    }

    if (offset + available > ilen) {
        available = ilen - offset;

        if (available < 0) {
            available = 0;
        }
    }

    memcpy(buf, string + offset, available);
    lua_pop(self->L, 3);
    return (uLong)available;
}

static uLong zip_write(voidpf const opaque, voidpf const stream, void const* const buf, uLong const size) {
    (void)opaque;
    (void)stream;
    (void)buf;
    (void)size;

    return -1;
}

static long zip_tell(voidpf const opaque, voidpf const stream) {
    (void)opaque;

    Unzip* const self = (Unzip*)stream;

    lua_rawgeti(self->L, LUA_REGISTRYINDEX, self->object_ref);
    lua_getfield(self->L, -1, "seek");
    lua_insert(self->L, -2);

    lua_call(self->L, 1, 1);

    lua_Integer pos = lua_tointeger(self->L, -1);
    lua_pop(self->L, 1);
    return (long)pos;
}

static long zip_seek(voidpf const opaque, voidpf const stream, uLong const offset, int const origin) {
    (void)opaque;

    Unzip* const self = (Unzip*)stream;

    lua_rawgeti(self->L, LUA_REGISTRYINDEX, self->object_ref);
    lua_getfield(self->L, -1, "seek");
    lua_insert(self->L, -2);

    switch (origin) {
        case ZLIB_FILEFUNC_SEEK_CUR: lua_pushliteral(self->L, "cur"); break;
        case ZLIB_FILEFUNC_SEEK_END: lua_pushliteral(self->L, "end"); break;
        case ZLIB_FILEFUNC_SEEK_SET: lua_pushliteral(self->L, "set"); break;

        default:
            lua_pop(self->L, 2);
            return -1;
    }

    lua_pushinteger(self->L, offset);
    lua_call(self->L, 3, 1);

    int const ok = !lua_isnil(self->L, -1);
    lua_pop(self->L, 1);
    return ok ? 0 : -1;
}

static int zip_close(voidpf const opaque, voidpf const stream) {
    (void)opaque;
    (void)stream;

    return 0;
}

static int zip_error(voidpf const opaque, voidpf const stream) {
    (void)opaque;
    (void)stream;

    return 0;
}

static int l_init(lua_State* const L) {
    Unzip* const self = (Unzip*)lua_newuserdata(L, sizeof(Unzip));

    self->io.zopen_file = zip_open;
    self->io.zread_file = zip_read;
    self->io.zwrite_file = zip_write;
    self->io.ztell_file = zip_tell;
    self->io.zseek_file = zip_seek;
    self->io.zclose_file = zip_close;
    self->io.zerror_file = zip_error;
    self->io.opaque = (voidpf)self;

    lua_pushvalue(L, 1);
    self->object_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    self->L = L;

    self->file = unzOpen2(NULL, &self->io);

    if (self->file == NULL) {
        lua_pushnil(L);
        lua_pushliteral(L, "error opening zip");
        return 2;
    }

    if (luaL_newmetatable(L, UNZIP_MT)) {
        static const luaL_Reg methods[] = {
            {"exists", l_exists},
            {"read", l_read},
            {NULL, NULL}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_gc);
        lua_setfield(L, -2, "__gc");
    }

    lua_setmetatable(L, -2);
    return 1;
}

static int l_crc32(lua_State* const L) {
    size_t length;
    char const* const string = luaL_checklstring(L, 1, &length);

    unsigned long crc32(unsigned long crc, const unsigned char FAR *buf, uInt len);
    lua_pushinteger(L, crc32(0, (const unsigned char FAR*)string, length));

    return 1;
}

LUAMOD_API int luaopen_unzip(lua_State* const L) {
    static const luaL_Reg functions[] = {
        {"init", l_init},
        {"crc32", l_crc32},
        {NULL, NULL}
    };

    luaL_newlib(L, functions);
    return 1;
}
