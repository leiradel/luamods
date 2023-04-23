#include <lua.hpp>

#include <lrcpp/Frontend.h>

#include <vector>
#include <stdlib.h>

#define FRONTEND_MT "lrcpp::Frontend"

static int l_loadGame(lua_State* const L) {
    lrcpp::Frontend& self = lrcpp::Frontend::getInstance();
    bool ok = false;

    if (lua_isstring(L, 2)) {
        char const* const path = luaL_checkstring(L, 2);

        if (lua_isstring(L, 3)) {
            size_t size = 0;
            char const* const data = luaL_checklstring(L, 3, &size);

            ok = self.loadGame(path, (void const*)data, size);
        }
        else {
            ok = self.loadGame(path);
        }
    }
    else {
        ok = self.loadGame();
    }

    lua_pushboolean(L, ok);
    return 1;
}

static int l_loadGameSpecial(lua_State* const L) {
    lrcpp::Frontend& self = lrcpp::Frontend::getInstance();
    lua_Integer const game_type = luaL_checkinteger(L, 2);

    int const top = lua_gettop(L);
    std::vector<retro_game_info> infos;

    for (lua_Integer i = 3; i <= top; i++) {
        retro_game_info info = {0};

        lua_getfield(L, 3, "path");
        info.path = luaL_checkstring(L, -1);

        lua_getfield(L, 3, "data");
        info.data = (void const*)luaL_checklstring(L, -1, &info.size);

        lua_getfield(L, 3, "meta");
        info.meta = luaL_checkstring(L, -1);

        infos.emplace_back(info);
    }

    bool const ok = self.loadGameSpecial(game_type, infos.data(), infos.size());

    lua_pushboolean(L, ok);
    return 1;
}

#define NO_ARG(S) \
    static int l_ ## S(lua_State* const L) { \
        lrcpp::Frontend& self = lrcpp::Frontend::getInstance(); \
        bool const ok = self.S(); \
        lua_pushboolean(L, ok); \
        return 1; \
    }

NO_ARG(run)
NO_ARG(reset)
NO_ARG(unloadGame)

#define ARG_UNSIGNED_PTR(S) \
    static int l_ ## S(lua_State* const L) { \
        lrcpp::Frontend& self = lrcpp::Frontend::getInstance(); \
        unsigned res = 0; \
        bool const ok = self.S(&res); \
        lua_pushboolean(L, ok); \
        if (ok) { lua_pushinteger(L, res); return 2; } \
        return 1; \
    }

ARG_UNSIGNED_PTR(apiVersion)
NO_ARG(cheatReset)
ARG_UNSIGNED_PTR(getRegion)

static int l_getSystemInfo(lua_State* const L) {
    lrcpp::Frontend& self = lrcpp::Frontend::getInstance();

    retro_system_info info = {0};
    bool const ok = self.getSystemInfo(&info);

    lua_pushboolean(L, ok);

    if (ok) {
        lua_pushstring(L, info.library_name);
        lua_pushstring(L, info.library_version);
        lua_pushstring(L, info.valid_extensions);
        lua_pushboolean(L, info.need_fullpath);
        lua_pushboolean(L, info.block_extract);

        return 6;
    }

    return 1;
}

static int l_getSystemAvInfo(lua_State* const L) {
    lrcpp::Frontend& self = lrcpp::Frontend::getInstance();

    retro_system_av_info info = {0};
    bool const ok = self.getSystemAvInfo(&info);

    lua_pushboolean(L, ok);

    if (ok) {
        lua_pushinteger(L, info.geometry.base_width);
        lua_pushinteger(L, info.geometry.base_height);
        lua_pushinteger(L, info.geometry.max_width);
        lua_pushinteger(L, info.geometry.max_height);
        lua_pushnumber(L, info.geometry.aspect_ratio);

        lua_pushnumber(L, info.timing.fps);
        lua_pushnumber(L, info.timing.sample_rate);

        return 8;
    }

    return 1;
}

static int l_serializeSize(lua_State* const L) {
    lrcpp::Frontend& self = lrcpp::Frontend::getInstance();

    size_t size = 0;
    bool const ok = self.serializeSize(&size);

    lua_pushboolean(L, ok);

    if (ok) {
        lua_pushinteger(L, size);
        return 2;
    }

    return 1;
}

static int l_serialize(lua_State* const L) {
    lrcpp::Frontend& self = lrcpp::Frontend::getInstance();

    size_t size = 0;
    bool const ok1 = self.serializeSize(&size);

    if (!ok1) {
        lua_pushboolean(L, 0);
        return 1;
    }

    void* const data = malloc(size);

    if (data == nullptr) {
        lua_pushboolean(L, 0);
        return 1;
    }

    bool const ok2 = self.serialize(data, size);

    if (!ok2) {
        free(data);
        lua_pushboolean(L, 0);
        return 1;
    }

    lua_pushboolean(L, 1);
    lua_pushlstring(L, (char const*)data, size);

    free(data);
    return 2;
}

static int l_unserialize(lua_State* const L) {
    lrcpp::Frontend& self = lrcpp::Frontend::getInstance();

    size_t size = 0;
    char const* const data = luaL_checklstring(L, 2, &size);

    bool const ok = self.unserialize((void const*)data, size);
    free((void*)data);

    lua_pushboolean(L, ok);
    return 1;
}

static int l_cheatSet(lua_State* const L) {
    lrcpp::Frontend& self = lrcpp::Frontend::getInstance();
    lua_Integer const index = luaL_checkinteger(L, 2);
    bool const enabled = lua_toboolean(L, 3) != 0;
    char const* const code = luaL_checkstring(L, 4);

    bool const ok = self.cheatSet(index, enabled, code);

    lua_pushboolean(L, ok);
    return 1;
}

static int l_getMemoryData(lua_State* const L) {
    lrcpp::Frontend& self = lrcpp::Frontend::getInstance();
    lua_Integer const id = luaL_checkinteger(L, 2);

    size_t size = 0;
    bool const ok1 = self.getMemorySize(id, &size);

    if (!ok1) {
        lua_pushboolean(L, 0);
        return 1;
    }

    void* data = nullptr;
    bool const ok2 = self.getMemoryData(id, &data);

    if (!ok2) {
        lua_pushboolean(L, 0);
        return 1;
    }

    lua_pushboolean(L, 1);
    lua_pushlstring(L, (char const*)data, size);
    return 2;
}

static int l_getMemorySize(lua_State* const L) {
    lrcpp::Frontend& self = lrcpp::Frontend::getInstance();
    lua_Integer const id = luaL_checkinteger(L, 2);

    size_t size = 0;
    bool const ok = self.getMemorySize(id, &size);

    lua_pushboolean(L, ok);

    if (ok) {
        lua_pushinteger(L, size);
        return 2;
    }

    return 1;
}

static int l_setControllerPortDevice(lua_State* const L) {
    lrcpp::Frontend& self = lrcpp::Frontend::getInstance();
    lua_Integer const port = luaL_checkinteger(L, 2);
    lua_Integer const device = luaL_checkinteger(L, 3);

    bool const ok = self.setControllerPortDevice(port, device);

    lua_pushboolean(L, ok);
    return 1;
}

int lrcpp_push(lua_State* L, lrcpp::Frontend* frontend) {
    Frontend** self = (Frontend**)lua_newuserdata(L, sizeof(Frontend*));
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

int lrcpp_openf(lua_State* L) {
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
