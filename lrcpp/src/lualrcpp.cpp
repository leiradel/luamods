#include <lualrcpp.h>

#include <vector>
#include <stdlib.h>

#define FRONTEND_MT "lrcpp::Frontend"

lrcpp::Frontend* lrcpp::check(lua_State* const L, int const ndx) {
    auto const self = *(lrcpp::Frontend**)luaL_checkudata(L, ndx, FRONTEND_MT);
    return self;
}

static int l_loadGame(lua_State* const L) {
    auto const self = lrcpp::check(L, 1);
    bool ok = false;

    if (lua_isstring(L, 2)) {
        char const* const path = luaL_checkstring(L, 2);

        if (lua_isstring(L, 3)) {
            size_t size = 0;
            char const* const data = luaL_checklstring(L, 3, &size);

            ok = self->loadGame(path, (void const*)data, size);
        }
        else {
            ok = self->loadGame(path);
        }
    }
    else {
        ok = self->loadGame();
    }

    if (!ok) {
        return luaL_error(L, "error in %s", "loadGame");
    }

    return 0;
}

static int l_loadGameSpecial(lua_State* const L) {
    auto const self = lrcpp::check(L, 1);
    lua_Integer const game_type = luaL_checkinteger(L, 2);

    int const top = lua_gettop(L);
    std::vector<retro_game_info> infos;

    for (lua_Integer i = 3; i <= top; i++) {
        retro_game_info info = {};

        lua_getfield(L, 3, "path");
        info.path = luaL_checkstring(L, -1);

        lua_getfield(L, 3, "data");
        info.data = (void const*)luaL_checklstring(L, -1, &info.size);

        lua_getfield(L, 3, "meta");
        info.meta = luaL_checkstring(L, -1);

        infos.emplace_back(info);
    }

    if (!self->loadGameSpecial(game_type, infos.data(), infos.size())) {
        return luaL_error(L, "error in %s", "loadGameSpecial");
    }

    return 0;
}

#define NO_ARG(S) \
    static int l_ ## S(lua_State* const L) { \
        auto const self = lrcpp::check(L, 1); \
        if (!self->S()) return luaL_error(L, "error in %s", #S); \
        return 0; \
    }

NO_ARG(run)
NO_ARG(reset)
NO_ARG(unloadGame)

#define ARG_UNSIGNED_PTR(S) \
    static int l_ ## S(lua_State* const L) { \
        auto const self = lrcpp::check(L, 1); \
        unsigned res = 0; \
        if (!self->S(&res)) return luaL_error(L, "error in %s", #S); \
        lua_pushinteger(L, res); \
        return 1; \
    }

ARG_UNSIGNED_PTR(apiVersion)
NO_ARG(cheatReset)
ARG_UNSIGNED_PTR(getRegion)

static int l_getSystemInfo(lua_State* const L) {
    auto const self = lrcpp::check(L, 1);

    retro_system_info info = {};

    if (!self->getSystemInfo(&info)) {
        return luaL_error(L, "error in %s", "getSystemInfo");
    }

    lua_pushstring(L, info.library_name);
    lua_pushstring(L, info.library_version);
    lua_pushstring(L, info.valid_extensions);
    lua_pushboolean(L, info.need_fullpath);
    lua_pushboolean(L, info.block_extract);

    return 5;
}

static int l_getSystemAvInfo(lua_State* const L) {
    auto const self = lrcpp::check(L, 1);

    retro_system_av_info info = {};

    if (!self->getSystemAvInfo(&info)) {
        return luaL_error(L, "error in %s", "getSystemAvInfo");
    }

    lua_pushinteger(L, info.geometry.base_width);
    lua_pushinteger(L, info.geometry.base_height);
    lua_pushinteger(L, info.geometry.max_width);
    lua_pushinteger(L, info.geometry.max_height);
    lua_pushnumber(L, info.geometry.aspect_ratio);

    lua_pushnumber(L, info.timing.fps);
    lua_pushnumber(L, info.timing.sample_rate);

    return 7;
}

static int l_serializeSize(lua_State* const L) {
    auto const self = lrcpp::check(L, 1);

    size_t size = 0;

    if (!self->serializeSize(&size)) {
        return luaL_error(L, "error in %s", "serializeSize");
    }

    lua_pushinteger(L, size);
    return 1;
}

static int l_serialize(lua_State* const L) {
    auto const self = lrcpp::check(L, 1);

    size_t size = 0;

    if (!self->serializeSize(&size)) {
        return luaL_error(L, "error in %s", "serializeSize");
    }

    void* const data = malloc(size);

    if (data == nullptr) {
        return luaL_error(L, "error allocating memory for the state");
    }

    if (!self->serialize(data, size)) {
        free(data);
        return luaL_error(L, "error in %s", "serialize");
    }

    lua_pushlstring(L, (char const*)data, size);
    return 1;
}

static int l_unserialize(lua_State* const L) {
    auto const self = lrcpp::check(L, 1);

    size_t size = 0;
    char const* const data = luaL_checklstring(L, 2, &size);

    if (!self->unserialize((void const*)data, size)) {
        return luaL_error(L, "error in %s", "unserialize");
    }

    return 0;
}

static int l_cheatSet(lua_State* const L) {
    auto const self = lrcpp::check(L, 1);
    lua_Integer const index = luaL_checkinteger(L, 2);
    bool const enabled = lua_toboolean(L, 3) != 0;
    char const* const code = luaL_checkstring(L, 4);

    if (!self->cheatSet(index, enabled, code)) {
        return luaL_error(L, "error in %s", "cheatSet");
    }

    return 0;
}

static int l_getMemoryData(lua_State* const L) {
    auto const self = lrcpp::check(L, 1);
    lua_Integer const id = luaL_checkinteger(L, 2);

    size_t size = 0;

    if (!self->getMemorySize(id, &size)) {
        return luaL_error(L, "error in %s", "getMemorySize");
    }

    void* data = nullptr;

    if (!self->getMemoryData(id, &data)) {
        return luaL_error(L, "error in %s", "getMemoryData");
    }

    lua_pushlstring(L, (char const*)data, size);
    return 1;
}

static int l_getMemorySize(lua_State* const L) {
    auto const self = lrcpp::check(L, 1);
    lua_Integer const id = luaL_checkinteger(L, 2);

    size_t size = 0;

    if (!self->getMemorySize(id, &size)) {
        return luaL_error(L, "error in %s", "getMemorySize");
    }

    lua_pushinteger(L, size);
    return 1;
}

static int l_setControllerPortDevice(lua_State* const L) {
    auto const self = lrcpp::check(L, 1);
    lua_Integer const port = luaL_checkinteger(L, 2);
    lua_Integer const device = luaL_checkinteger(L, 3);

    if (!self->setControllerPortDevice(port, device)) {
        return luaL_error(L, "error in %s", "setControllerPortDevice");
    }

    return 0;
}

int lrcpp::push(lua_State* const L, lrcpp::Frontend* frontend) {
    auto const self = (lrcpp::Frontend**)lua_newuserdata(L, sizeof(lrcpp::Frontend*));
    *self = frontend;

    if (luaL_newmetatable(L, FRONTEND_MT)) {
        static luaL_Reg const methods[] = {
            // Core life-cycle
            {"loadGame", l_loadGame},
            {"loadGameSpecial", l_loadGameSpecial},
            {"run", l_run},
            {"reset", l_reset},
            {"unloadGame", l_unloadGame},
            // Other Libretro API calls
            {"apiVersion", l_apiVersion},
            {"getSystemInfo", l_getSystemInfo},
            {"getSystemAvInfo", l_getSystemAvInfo},
            {"serializeSize", l_serializeSize},
            {"serialize", l_serialize},
            {"unserialize", l_unserialize},
            {"cheatReset", l_cheatReset},
            {"cheatSet", l_cheatSet},
            {"getRegion", l_getRegion},
            {"getMemoryData", l_getMemoryData},
            {"getMemorySize", l_getMemorySize},
            {"setControllerPortDevice", l_setControllerPortDevice},
            {nullptr, nullptr}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

int lrcpp::openf(lua_State* const L) {
    static struct {char const* const name; lua_Integer const value;} const constants[] = {
        {"RETRO_MEMORY_SAVE_RAM", RETRO_MEMORY_SAVE_RAM},
        {"RETRO_MEMORY_RTC", RETRO_MEMORY_RTC},
        {"RETRO_MEMORY_SYSTEM_RAM", RETRO_MEMORY_SYSTEM_RAM},
        {"RETRO_MEMORY_VIDEO_RAM", RETRO_MEMORY_VIDEO_RAM}
    };

    static struct {char const* const name; char const* const value;} const info[] = {
        {"_COPYRIGHT", "Copyright (c) 2023 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "1.0.0"},
        {"_NAME", "lrcpp"},
        {"_URL", "https://github.com/leiradel/luamods/lrcpp"},
        {"_DESCRIPTION", "Lua bindings to lrcpp"}
    };

    size_t const constants_count = sizeof(constants) / sizeof(constants[0]);
    size_t const info_count = sizeof(info) / sizeof(info[0]);

    lua_createtable(L, 0, constants_count + info_count);

    for (size_t i = 0; i < constants_count; i++) {
        lua_pushinteger(L, constants[i].value);
        lua_setfield(L, -2, constants[i].name);
    }

    for (size_t i = 0; i < info_count; i++) {
        lua_pushstring(L, info[i].value);
        lua_setfield(L, -2, info[i].name);
    }

    return 1;
}
