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
        {"RETRO_DEVICE_TYPE_SHIFT", RETRO_DEVICE_TYPE_SHIFT},
        {"RETRO_DEVICE_MASK", RETRO_DEVICE_MASK},
        {"RETRO_DEVICE_NONE", RETRO_DEVICE_NONE},
        {"RETRO_DEVICE_JOYPAD", RETRO_DEVICE_JOYPAD},
        {"RETRO_DEVICE_MOUSE", RETRO_DEVICE_MOUSE},
        {"RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK", RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK},
        {"RETRO_DEVICE_KEYBOARD", RETRO_DEVICE_KEYBOARD},
        {"RETRO_DEVICE_LIGHTGUN", RETRO_DEVICE_LIGHTGUN},
        {"RETRO_DEVICE_ANALOG", RETRO_DEVICE_ANALOG},
        {"RETRO_DEVICE_POINTER", RETRO_DEVICE_POINTER},
        {"RETRO_DEVICE_INDEX_ANALOG_BUTTON", RETRO_DEVICE_INDEX_ANALOG_BUTTON},
        {"RETRO_DEVICE_ID_JOYPAD_B", RETRO_DEVICE_ID_JOYPAD_B},
        {"RETRO_DEVICE_ID_JOYPAD_Y", RETRO_DEVICE_ID_JOYPAD_Y},
        {"RETRO_DEVICE_ID_JOYPAD_SELECT", RETRO_DEVICE_ID_JOYPAD_SELECT},
        {"RETRO_DEVICE_ID_JOYPAD_START", RETRO_DEVICE_ID_JOYPAD_START},
        {"RETRO_DEVICE_ID_JOYPAD_UP", RETRO_DEVICE_ID_JOYPAD_UP},
        {"RETRO_DEVICE_ID_JOYPAD_DOWN", RETRO_DEVICE_ID_JOYPAD_DOWN},
        {"RETRO_DEVICE_ID_JOYPAD_LEFT", RETRO_DEVICE_ID_JOYPAD_LEFT},
        {"RETRO_DEVICE_ID_JOYPAD_RIGHT", RETRO_DEVICE_ID_JOYPAD_RIGHT},
        {"RETRO_DEVICE_ID_JOYPAD_A", RETRO_DEVICE_ID_JOYPAD_A},
        {"RETRO_DEVICE_ID_JOYPAD_X", RETRO_DEVICE_ID_JOYPAD_X},
        {"RETRO_DEVICE_ID_JOYPAD_L", RETRO_DEVICE_ID_JOYPAD_L},
        {"RETRO_DEVICE_ID_JOYPAD_R", RETRO_DEVICE_ID_JOYPAD_R},
        {"RETRO_DEVICE_ID_JOYPAD_L2", RETRO_DEVICE_ID_JOYPAD_L2},
        {"RETRO_DEVICE_ID_JOYPAD_R2", RETRO_DEVICE_ID_JOYPAD_R2},
        {"RETRO_DEVICE_ID_JOYPAD_L3", RETRO_DEVICE_ID_JOYPAD_L3},
        {"RETRO_DEVICE_ID_JOYPAD_R3", RETRO_DEVICE_ID_JOYPAD_R3},
        {"RETRO_DEVICE_ID_JOYPAD_MASK", RETRO_DEVICE_ID_JOYPAD_MASK},
        {"RETRO_DEVICE_INDEX_ANALOG_LEFT", RETRO_DEVICE_INDEX_ANALOG_LEFT},
        {"RETRO_DEVICE_INDEX_ANALOG_RIGHT", RETRO_DEVICE_INDEX_ANALOG_RIGHT},
        {"RETRO_DEVICE_INDEX_ANALOG_BUTTON", RETRO_DEVICE_INDEX_ANALOG_BUTTON},
        {"RETRO_DEVICE_ID_ANALOG_X", RETRO_DEVICE_ID_ANALOG_X},
        {"RETRO_DEVICE_ID_ANALOG_Y", RETRO_DEVICE_ID_ANALOG_Y},
        {"RETRO_DEVICE_ID_MOUSE_X", RETRO_DEVICE_ID_MOUSE_X},
        {"RETRO_DEVICE_ID_MOUSE_Y", RETRO_DEVICE_ID_MOUSE_Y},
        {"RETRO_DEVICE_ID_MOUSE_LEFT", RETRO_DEVICE_ID_MOUSE_LEFT},
        {"RETRO_DEVICE_ID_MOUSE_RIGHT", RETRO_DEVICE_ID_MOUSE_RIGHT},
        {"RETRO_DEVICE_ID_MOUSE_WHEELUP", RETRO_DEVICE_ID_MOUSE_WHEELUP},
        {"RETRO_DEVICE_ID_MOUSE_WHEELDOWN", RETRO_DEVICE_ID_MOUSE_WHEELDOWN},
        {"RETRO_DEVICE_ID_MOUSE_MIDDLE", RETRO_DEVICE_ID_MOUSE_MIDDLE},
        {"RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP", RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP},
        {"RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN", RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN},
        {"RETRO_DEVICE_ID_MOUSE_BUTTON_4", RETRO_DEVICE_ID_MOUSE_BUTTON_4},
        {"RETRO_DEVICE_ID_MOUSE_BUTTON_5", RETRO_DEVICE_ID_MOUSE_BUTTON_5},
        {"RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X", RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X},
        {"RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y", RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y},
        {"RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN", RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN},
        {"RETRO_DEVICE_ID_LIGHTGUN_TRIGGER", RETRO_DEVICE_ID_LIGHTGUN_TRIGGER},
        {"RETRO_DEVICE_ID_LIGHTGUN_RELOAD", RETRO_DEVICE_ID_LIGHTGUN_RELOAD},
        {"RETRO_DEVICE_ID_LIGHTGUN_AUX_A", RETRO_DEVICE_ID_LIGHTGUN_AUX_A},
        {"RETRO_DEVICE_ID_LIGHTGUN_AUX_B", RETRO_DEVICE_ID_LIGHTGUN_AUX_B},
        {"RETRO_DEVICE_ID_LIGHTGUN_START", RETRO_DEVICE_ID_LIGHTGUN_START},
        {"RETRO_DEVICE_ID_LIGHTGUN_SELECT", RETRO_DEVICE_ID_LIGHTGUN_SELECT},
        {"RETRO_DEVICE_ID_LIGHTGUN_AUX_C", RETRO_DEVICE_ID_LIGHTGUN_AUX_C},
        {"RETRO_DEVICE_ID_LIGHTGUN_DPAD_UP", RETRO_DEVICE_ID_LIGHTGUN_DPAD_UP},
        {"RETRO_DEVICE_ID_LIGHTGUN_DPAD_DOWN", RETRO_DEVICE_ID_LIGHTGUN_DPAD_DOWN},
        {"RETRO_DEVICE_ID_LIGHTGUN_DPAD_LEFT", RETRO_DEVICE_ID_LIGHTGUN_DPAD_LEFT},
        {"RETRO_DEVICE_ID_LIGHTGUN_DPAD_RIGHT", RETRO_DEVICE_ID_LIGHTGUN_DPAD_RIGHT},
        {"RETRO_DEVICE_ID_LIGHTGUN_X", RETRO_DEVICE_ID_LIGHTGUN_X},
        {"RETRO_DEVICE_ID_LIGHTGUN_Y", RETRO_DEVICE_ID_LIGHTGUN_Y},
        {"RETRO_DEVICE_ID_LIGHTGUN_CURSOR", RETRO_DEVICE_ID_LIGHTGUN_CURSOR},
        {"RETRO_DEVICE_ID_LIGHTGUN_TURBO", RETRO_DEVICE_ID_LIGHTGUN_TURBO},
        {"RETRO_DEVICE_ID_LIGHTGUN_PAUSE", RETRO_DEVICE_ID_LIGHTGUN_PAUSE},
        {"RETRO_DEVICE_ID_POINTER_X", RETRO_DEVICE_ID_POINTER_X},
        {"RETRO_DEVICE_ID_POINTER_Y", RETRO_DEVICE_ID_POINTER_Y},
        {"RETRO_DEVICE_ID_POINTER_PRESSED", RETRO_DEVICE_ID_POINTER_PRESSED},
        {"RETRO_DEVICE_ID_POINTER_COUNT", RETRO_DEVICE_ID_POINTER_COUNT},
        {"RETRO_REGION_NTSC", RETRO_REGION_NTSC},
        {"RETRO_REGION_PAL", RETRO_REGION_PAL},
        {"RETRO_LANGUAGE_ENGLISH", RETRO_LANGUAGE_ENGLISH},
        {"RETRO_LANGUAGE_JAPANESE", RETRO_LANGUAGE_JAPANESE},
        {"RETRO_LANGUAGE_FRENCH", RETRO_LANGUAGE_FRENCH},
        {"RETRO_LANGUAGE_SPANISH", RETRO_LANGUAGE_SPANISH},
        {"RETRO_LANGUAGE_GERMAN", RETRO_LANGUAGE_GERMAN},
        {"RETRO_LANGUAGE_ITALIAN", RETRO_LANGUAGE_ITALIAN},
        {"RETRO_LANGUAGE_DUTCH", RETRO_LANGUAGE_DUTCH},
        {"RETRO_LANGUAGE_PORTUGUESE_BRAZIL", RETRO_LANGUAGE_PORTUGUESE_BRAZIL},
        {"RETRO_LANGUAGE_PORTUGUESE_PORTUGAL", RETRO_LANGUAGE_PORTUGUESE_PORTUGAL},
        {"RETRO_LANGUAGE_RUSSIAN", RETRO_LANGUAGE_RUSSIAN},
        {"RETRO_LANGUAGE_KOREAN", RETRO_LANGUAGE_KOREAN},
        {"RETRO_LANGUAGE_CHINESE_TRADITIONAL", RETRO_LANGUAGE_CHINESE_TRADITIONAL},
        {"RETRO_LANGUAGE_CHINESE_SIMPLIFIED", RETRO_LANGUAGE_CHINESE_SIMPLIFIED},
        {"RETRO_LANGUAGE_ESPERANTO", RETRO_LANGUAGE_ESPERANTO},
        {"RETRO_LANGUAGE_POLISH", RETRO_LANGUAGE_POLISH},
        {"RETRO_LANGUAGE_VIETNAMESE", RETRO_LANGUAGE_VIETNAMESE},
        {"RETRO_LANGUAGE_ARABIC", RETRO_LANGUAGE_ARABIC},
        {"RETRO_LANGUAGE_GREEK", RETRO_LANGUAGE_GREEK},
        {"RETRO_LANGUAGE_TURKISH", RETRO_LANGUAGE_TURKISH},
        {"RETRO_LANGUAGE_LAST", RETRO_LANGUAGE_LAST},
        {"RETRO_LANGUAGE_DUMMY", RETRO_LANGUAGE_DUMMY},
        {"RETRO_MEMORY_MASK", RETRO_MEMORY_MASK},
        {"RETRO_MEMORY_SAVE_RAM", RETRO_MEMORY_SAVE_RAM},
        {"RETRO_MEMORY_RTC", RETRO_MEMORY_RTC},
        {"RETRO_MEMORY_SYSTEM_RAM", RETRO_MEMORY_SYSTEM_RAM},
        {"RETRO_MEMORY_VIDEO_RAM", RETRO_MEMORY_VIDEO_RAM},
        {"RETROK_UNKNOWN", RETROK_UNKNOWN},
        {"RETROK_FIRST", RETROK_FIRST},
        {"RETROK_BACKSPACE", RETROK_BACKSPACE},
        {"RETROK_TAB", RETROK_TAB},
        {"RETROK_CLEAR", RETROK_CLEAR},
        {"RETROK_RETURN", RETROK_RETURN},
        {"RETROK_PAUSE", RETROK_PAUSE},
        {"RETROK_ESCAPE", RETROK_ESCAPE},
        {"RETROK_SPACE", RETROK_SPACE},
        {"RETROK_EXCLAIM", RETROK_EXCLAIM},
        {"RETROK_QUOTEDBL", RETROK_QUOTEDBL},
        {"RETROK_HASH", RETROK_HASH},
        {"RETROK_DOLLAR", RETROK_DOLLAR},
        {"RETROK_AMPERSAND", RETROK_AMPERSAND},
        {"RETROK_QUOTE", RETROK_QUOTE},
        {"RETROK_LEFTPAREN", RETROK_LEFTPAREN},
        {"RETROK_RIGHTPAREN", RETROK_RIGHTPAREN},
        {"RETROK_ASTERISK", RETROK_ASTERISK},
        {"RETROK_PLUS", RETROK_PLUS},
        {"RETROK_COMMA", RETROK_COMMA},
        {"RETROK_MINUS", RETROK_MINUS},
        {"RETROK_PERIOD", RETROK_PERIOD},
        {"RETROK_SLASH", RETROK_SLASH},
        {"RETROK_0", RETROK_0},
        {"RETROK_1", RETROK_1},
        {"RETROK_2", RETROK_2},
        {"RETROK_3", RETROK_3},
        {"RETROK_4", RETROK_4},
        {"RETROK_5", RETROK_5},
        {"RETROK_6", RETROK_6},
        {"RETROK_7", RETROK_7},
        {"RETROK_8", RETROK_8},
        {"RETROK_9", RETROK_9},
        {"RETROK_COLON", RETROK_COLON},
        {"RETROK_SEMICOLON", RETROK_SEMICOLON},
        {"RETROK_LESS", RETROK_LESS},
        {"RETROK_EQUALS", RETROK_EQUALS},
        {"RETROK_GREATER", RETROK_GREATER},
        {"RETROK_QUESTION", RETROK_QUESTION},
        {"RETROK_AT", RETROK_AT},
        {"RETROK_LEFTBRACKET", RETROK_LEFTBRACKET},
        {"RETROK_BACKSLASH", RETROK_BACKSLASH},
        {"RETROK_RIGHTBRACKET", RETROK_RIGHTBRACKET},
        {"RETROK_CARET", RETROK_CARET},
        {"RETROK_UNDERSCORE", RETROK_UNDERSCORE},
        {"RETROK_BACKQUOTE", RETROK_BACKQUOTE},
        {"RETROK_a", RETROK_a},
        {"RETROK_b", RETROK_b},
        {"RETROK_c", RETROK_c},
        {"RETROK_d", RETROK_d},
        {"RETROK_e", RETROK_e},
        {"RETROK_f", RETROK_f},
        {"RETROK_g", RETROK_g},
        {"RETROK_h", RETROK_h},
        {"RETROK_i", RETROK_i},
        {"RETROK_j", RETROK_j},
        {"RETROK_k", RETROK_k},
        {"RETROK_l", RETROK_l},
        {"RETROK_m", RETROK_m},
        {"RETROK_n", RETROK_n},
        {"RETROK_o", RETROK_o},
        {"RETROK_p", RETROK_p},
        {"RETROK_q", RETROK_q},
        {"RETROK_r", RETROK_r},
        {"RETROK_s", RETROK_s},
        {"RETROK_t", RETROK_t},
        {"RETROK_u", RETROK_u},
        {"RETROK_v", RETROK_v},
        {"RETROK_w", RETROK_w},
        {"RETROK_x", RETROK_x},
        {"RETROK_y", RETROK_y},
        {"RETROK_z", RETROK_z},
        {"RETROK_LEFTBRACE", RETROK_LEFTBRACE},
        {"RETROK_BAR", RETROK_BAR},
        {"RETROK_RIGHTBRACE", RETROK_RIGHTBRACE},
        {"RETROK_TILDE", RETROK_TILDE},
        {"RETROK_DELETE", RETROK_DELETE},
        {"RETROK_KP0", RETROK_KP0},
        {"RETROK_KP1", RETROK_KP1},
        {"RETROK_KP2", RETROK_KP2},
        {"RETROK_KP3", RETROK_KP3},
        {"RETROK_KP4", RETROK_KP4},
        {"RETROK_KP5", RETROK_KP5},
        {"RETROK_KP6", RETROK_KP6},
        {"RETROK_KP7", RETROK_KP7},
        {"RETROK_KP8", RETROK_KP8},
        {"RETROK_KP9", RETROK_KP9},
        {"RETROK_KP_PERIOD", RETROK_KP_PERIOD},
        {"RETROK_KP_DIVIDE", RETROK_KP_DIVIDE},
        {"RETROK_KP_MULTIPLY", RETROK_KP_MULTIPLY},
        {"RETROK_KP_MINUS", RETROK_KP_MINUS},
        {"RETROK_KP_PLUS", RETROK_KP_PLUS},
        {"RETROK_KP_ENTER", RETROK_KP_ENTER},
        {"RETROK_KP_EQUALS", RETROK_KP_EQUALS},
        {"RETROK_UP", RETROK_UP},
        {"RETROK_DOWN", RETROK_DOWN},
        {"RETROK_RIGHT", RETROK_RIGHT},
        {"RETROK_LEFT", RETROK_LEFT},
        {"RETROK_INSERT", RETROK_INSERT},
        {"RETROK_HOME", RETROK_HOME},
        {"RETROK_END", RETROK_END},
        {"RETROK_PAGEUP", RETROK_PAGEUP},
        {"RETROK_PAGEDOWN", RETROK_PAGEDOWN},
        {"RETROK_F1", RETROK_F1},
        {"RETROK_F2", RETROK_F2},
        {"RETROK_F3", RETROK_F3},
        {"RETROK_F4", RETROK_F4},
        {"RETROK_F5", RETROK_F5},
        {"RETROK_F6", RETROK_F6},
        {"RETROK_F7", RETROK_F7},
        {"RETROK_F8", RETROK_F8},
        {"RETROK_F9", RETROK_F9},
        {"RETROK_F10", RETROK_F10},
        {"RETROK_F11", RETROK_F11},
        {"RETROK_F12", RETROK_F12},
        {"RETROK_F13", RETROK_F13},
        {"RETROK_F14", RETROK_F14},
        {"RETROK_F15", RETROK_F15},
        {"RETROK_NUMLOCK", RETROK_NUMLOCK},
        {"RETROK_CAPSLOCK", RETROK_CAPSLOCK},
        {"RETROK_SCROLLOCK", RETROK_SCROLLOCK},
        {"RETROK_RSHIFT", RETROK_RSHIFT},
        {"RETROK_LSHIFT", RETROK_LSHIFT},
        {"RETROK_RCTRL", RETROK_RCTRL},
        {"RETROK_LCTRL", RETROK_LCTRL},
        {"RETROK_RALT", RETROK_RALT},
        {"RETROK_LALT", RETROK_LALT},
        {"RETROK_RMETA", RETROK_RMETA},
        {"RETROK_LMETA", RETROK_LMETA},
        {"RETROK_LSUPER", RETROK_LSUPER},
        {"RETROK_RSUPER", RETROK_RSUPER},
        {"RETROK_MODE", RETROK_MODE},
        {"RETROK_COMPOSE", RETROK_COMPOSE},
        {"RETROK_HELP", RETROK_HELP},
        {"RETROK_PRINT", RETROK_PRINT},
        {"RETROK_SYSREQ", RETROK_SYSREQ},
        {"RETROK_BREAK", RETROK_BREAK},
        {"RETROK_MENU", RETROK_MENU},
        {"RETROK_POWER", RETROK_POWER},
        {"RETROK_EURO", RETROK_EURO},
        {"RETROK_UNDO", RETROK_UNDO},
        {"RETROK_OEM_102", RETROK_OEM_102},
        {"RETROK_LAST", RETROK_LAST},
        {"RETROK_DUMMY", RETROK_DUMMY},
        {"RETROKMOD_NONE", RETROKMOD_NONE},
        {"RETROKMOD_SHIFT", RETROKMOD_SHIFT},
        {"RETROKMOD_CTRL", RETROKMOD_CTRL},
        {"RETROKMOD_ALT", RETROKMOD_ALT},
        {"RETROKMOD_META", RETROKMOD_META},
        {"RETROKMOD_NUMLOCK", RETROKMOD_NUMLOCK},
        {"RETROKMOD_CAPSLOCK", RETROKMOD_CAPSLOCK},
        {"RETROKMOD_SCROLLOCK", RETROKMOD_SCROLLOCK},
        {"RETROKMOD_DUMMY", RETROKMOD_DUMMY},
        {"RETRO_ENVIRONMENT_EXPERIMENTAL", RETRO_ENVIRONMENT_EXPERIMENTAL},
        {"RETRO_ENVIRONMENT_PRIVATE", RETRO_ENVIRONMENT_PRIVATE},
        {"RETRO_ENVIRONMENT_SET_ROTATION", RETRO_ENVIRONMENT_SET_ROTATION},
        {"RETRO_ENVIRONMENT_GET_OVERSCAN", RETRO_ENVIRONMENT_GET_OVERSCAN},
        {"RETRO_ENVIRONMENT_GET_CAN_DUPE", RETRO_ENVIRONMENT_GET_CAN_DUPE},
        {"RETRO_ENVIRONMENT_SET_MESSAGE", RETRO_ENVIRONMENT_SET_MESSAGE},
        {"RETRO_ENVIRONMENT_GET_LOG_INTERFACE", RETRO_ENVIRONMENT_GET_LOG_INTERFACE},
        {"RETRO_ENVIRONMENT_SHUTDOWN", RETRO_ENVIRONMENT_SHUTDOWN},
        {"RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL", RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL},
        {"RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY", RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY},
        {"RETRO_ENVIRONMENT_SET_PIXEL_FORMAT", RETRO_ENVIRONMENT_SET_PIXEL_FORMAT},
        {"RETRO_PIXEL_FORMAT_0RGB1555", RETRO_PIXEL_FORMAT_0RGB1555},
        {"RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS", RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS},
        {"RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK", RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK},
        {"RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE", RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE},
        {"RETRO_ENVIRONMENT_SET_HW_RENDER", RETRO_ENVIRONMENT_SET_HW_RENDER},
        {"RETRO_ENVIRONMENT_GET_VARIABLE", RETRO_ENVIRONMENT_GET_VARIABLE},
        {"RETRO_ENVIRONMENT_SET_VARIABLES", RETRO_ENVIRONMENT_SET_VARIABLES},
        {"RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE", RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE},
        {"RETRO_ENVIRONMENT_GET_VARIABLE", RETRO_ENVIRONMENT_GET_VARIABLE},
        {"RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME", RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME},
        {"RETRO_ENVIRONMENT_GET_LIBRETRO_PATH", RETRO_ENVIRONMENT_GET_LIBRETRO_PATH},
        {"RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK", RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK},
        {"RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK", RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK},
        {"RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE", RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE},
        {"RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES", RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES},
        {"RETRO_DEVICE_JOYPAD", RETRO_DEVICE_JOYPAD},
        {"RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE", RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE},
        {"RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE", RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE},
        {"RETRO_ENVIRONMENT_GET_LOG_INTERFACE", RETRO_ENVIRONMENT_GET_LOG_INTERFACE},
        {"RETRO_ENVIRONMENT_GET_PERF_INTERFACE", RETRO_ENVIRONMENT_GET_PERF_INTERFACE},
        {"RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE", RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE},
        {"RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY", RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY},
        {"RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY", RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY},
        {"RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY", RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY},
        {"RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO", RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO},
        {"RETRO_ENVIRONMENT_SET_GEOMETRY", RETRO_ENVIRONMENT_SET_GEOMETRY},
        {"RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK", RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK},
        {"RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO", RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO},
        {"RETRO_ENVIRONMENT_SET_CONTROLLER_INFO", RETRO_ENVIRONMENT_SET_CONTROLLER_INFO},
        {"RETRO_ENVIRONMENT_SET_MEMORY_MAPS", RETRO_ENVIRONMENT_SET_MEMORY_MAPS},
        {"RETRO_ENVIRONMENT_SET_GEOMETRY", RETRO_ENVIRONMENT_SET_GEOMETRY},
        {"RETRO_ENVIRONMENT_GET_USERNAME", RETRO_ENVIRONMENT_GET_USERNAME},
        {"RETRO_ENVIRONMENT_GET_LANGUAGE", RETRO_ENVIRONMENT_GET_LANGUAGE},
        {"RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER", RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER},
        {"RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE", RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE},
        {"RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS", RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS},
        {"RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE", RETRO_ENVIRONMENT_SET_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE},
        {"RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS", RETRO_ENVIRONMENT_SET_SERIALIZATION_QUIRKS},
        {"RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT", RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT},
        {"RETRO_ENVIRONMENT_GET_VFS_INTERFACE", RETRO_ENVIRONMENT_GET_VFS_INTERFACE},
        {"RETRO_ENVIRONMENT_GET_LED_INTERFACE", RETRO_ENVIRONMENT_GET_LED_INTERFACE},
        {"RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE", RETRO_ENVIRONMENT_GET_AUDIO_VIDEO_ENABLE},
        {"RETRO_ENVIRONMENT_GET_MIDI_INTERFACE", RETRO_ENVIRONMENT_GET_MIDI_INTERFACE},
        {"RETRO_ENVIRONMENT_GET_FASTFORWARDING", RETRO_ENVIRONMENT_GET_FASTFORWARDING},
        {"RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE", RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE},
        {"RETRO_ENVIRONMENT_GET_INPUT_BITMASKS", RETRO_ENVIRONMENT_GET_INPUT_BITMASKS},
        {"RETRO_DEVICE_ID_JOYPAD_MASK", RETRO_DEVICE_ID_JOYPAD_MASK},
        {"RETRO_DEVICE_JOYPAD", RETRO_DEVICE_JOYPAD},
        {"RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION", RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION},
        {"RETRO_ENVIRONMENT_SET_VARIABLES", RETRO_ENVIRONMENT_SET_VARIABLES},
        {"RETRO_ENVIRONMENT_SET_CORE_OPTIONS", RETRO_ENVIRONMENT_SET_CORE_OPTIONS},
        {"RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL", RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL},
        {"RETRO_ENVIRONMENT_SET_CORE_OPTIONS", RETRO_ENVIRONMENT_SET_CORE_OPTIONS},
        {"RETRO_ENVIRONMENT_SET_VARIABLES", RETRO_ENVIRONMENT_SET_VARIABLES},
        {"RETRO_NUM_CORE_OPTION_VALUES_MAX", RETRO_NUM_CORE_OPTION_VALUES_MAX},
        {"RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL", RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL},
        {"RETRO_ENVIRONMENT_SET_VARIABLES", RETRO_ENVIRONMENT_SET_VARIABLES},
        {"RETRO_ENVIRONMENT_SET_CORE_OPTIONS", RETRO_ENVIRONMENT_SET_CORE_OPTIONS},
        {"RETRO_ENVIRONMENT_SET_CORE_OPTIONS", RETRO_ENVIRONMENT_SET_CORE_OPTIONS},
        {"RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY", RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY},
        {"RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER", RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER},
        {"RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION", RETRO_ENVIRONMENT_GET_DISK_CONTROL_INTERFACE_VERSION},
        {"RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE", RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE},
        {"RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE", RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE},
        {"RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE", RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE},
        {"RETRO_VFS_FILE_ACCESS_READ", RETRO_VFS_FILE_ACCESS_READ},
        {"RETRO_VFS_FILE_ACCESS_WRITE", RETRO_VFS_FILE_ACCESS_WRITE},
        {"RETRO_VFS_FILE_ACCESS_READ_WRITE", RETRO_VFS_FILE_ACCESS_READ_WRITE},
        {"RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING", RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING},
        {"RETRO_VFS_FILE_ACCESS_HINT_NONE", RETRO_VFS_FILE_ACCESS_HINT_NONE},
        {"RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS", RETRO_VFS_FILE_ACCESS_HINT_FREQUENT_ACCESS},
        {"RETRO_VFS_SEEK_POSITION_START", RETRO_VFS_SEEK_POSITION_START},
        {"RETRO_VFS_SEEK_POSITION_CURRENT", RETRO_VFS_SEEK_POSITION_CURRENT},
        {"RETRO_VFS_SEEK_POSITION_END", RETRO_VFS_SEEK_POSITION_END},
        {"RETRO_VFS_STAT_IS_VALID", RETRO_VFS_STAT_IS_VALID},
        {"RETRO_VFS_STAT_IS_DIRECTORY", RETRO_VFS_STAT_IS_DIRECTORY},
        {"RETRO_VFS_STAT_IS_CHARACTER_SPECIAL", RETRO_VFS_STAT_IS_CHARACTER_SPECIAL},
        {"RETRO_ENVIRONMENT_GET_VFS_INTERFACE", RETRO_ENVIRONMENT_GET_VFS_INTERFACE},
        {"RETRO_HW_RENDER_INTERFACE_VULKAN", RETRO_HW_RENDER_INTERFACE_VULKAN},
        {"RETRO_HW_RENDER_INTERFACE_D3D9", RETRO_HW_RENDER_INTERFACE_D3D9},
        {"RETRO_HW_RENDER_INTERFACE_D3D10", RETRO_HW_RENDER_INTERFACE_D3D10},
        {"RETRO_HW_RENDER_INTERFACE_D3D11", RETRO_HW_RENDER_INTERFACE_D3D11},
        {"RETRO_HW_RENDER_INTERFACE_D3D12", RETRO_HW_RENDER_INTERFACE_D3D12},
        {"RETRO_HW_RENDER_INTERFACE_GSKIT_PS2", RETRO_HW_RENDER_INTERFACE_GSKIT_PS2},
        {"RETRO_HW_RENDER_INTERFACE_DUMMY", RETRO_HW_RENDER_INTERFACE_DUMMY},
        {"RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN", RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_VULKAN},
        {"RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_DUMMY", RETRO_HW_RENDER_CONTEXT_NEGOTIATION_INTERFACE_DUMMY},
        {"RETRO_SERIALIZATION_QUIRK_INCOMPLETE", RETRO_SERIALIZATION_QUIRK_INCOMPLETE},
        {"RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE", RETRO_SERIALIZATION_QUIRK_MUST_INITIALIZE},
        {"RETRO_SERIALIZATION_QUIRK_CORE_VARIABLE_SIZE", RETRO_SERIALIZATION_QUIRK_CORE_VARIABLE_SIZE},
        {"RETRO_SERIALIZATION_QUIRK_FRONT_VARIABLE_SIZE", RETRO_SERIALIZATION_QUIRK_FRONT_VARIABLE_SIZE},
        {"RETRO_SERIALIZATION_QUIRK_SINGLE_SESSION", RETRO_SERIALIZATION_QUIRK_SINGLE_SESSION},
        {"RETRO_SERIALIZATION_QUIRK_ENDIAN_DEPENDENT", RETRO_SERIALIZATION_QUIRK_ENDIAN_DEPENDENT},
        {"RETRO_SERIALIZATION_QUIRK_PLATFORM_DEPENDENT", RETRO_SERIALIZATION_QUIRK_PLATFORM_DEPENDENT},
        {"RETRO_MEMDESC_CONST", RETRO_MEMDESC_CONST},
        {"RETRO_MEMDESC_BIGENDIAN", RETRO_MEMDESC_BIGENDIAN},
        {"RETRO_MEMDESC_SYSTEM_RAM", RETRO_MEMDESC_SYSTEM_RAM},
        {"RETRO_MEMDESC_SAVE_RAM", RETRO_MEMDESC_SAVE_RAM},
        {"RETRO_MEMDESC_VIDEO_RAM", RETRO_MEMDESC_VIDEO_RAM},
        {"RETRO_MEMDESC_ALIGN_2", RETRO_MEMDESC_ALIGN_2},
        {"RETRO_MEMDESC_ALIGN_4", RETRO_MEMDESC_ALIGN_4},
        {"RETRO_MEMDESC_ALIGN_8", RETRO_MEMDESC_ALIGN_8},
        {"RETRO_MEMDESC_MINSIZE_2", RETRO_MEMDESC_MINSIZE_2},
        {"RETRO_MEMDESC_MINSIZE_4", RETRO_MEMDESC_MINSIZE_4},
        {"RETRO_MEMDESC_MINSIZE_8", RETRO_MEMDESC_MINSIZE_8},
        {"RETRO_LOG_DEBUG", RETRO_LOG_DEBUG},
        {"RETRO_LOG_INFO", RETRO_LOG_INFO},
        {"RETRO_LOG_WARN", RETRO_LOG_WARN},
        {"RETRO_LOG_ERROR", RETRO_LOG_ERROR},
        {"RETRO_LOG_DUMMY", RETRO_LOG_DUMMY},
        {"RETRO_SIMD_SSE", RETRO_SIMD_SSE},
        {"RETRO_SIMD_SSE2", RETRO_SIMD_SSE2},
        {"RETRO_SIMD_VMX", RETRO_SIMD_VMX},
        {"RETRO_SIMD_VMX128", RETRO_SIMD_VMX128},
        {"RETRO_SIMD_AVX", RETRO_SIMD_AVX},
        {"RETRO_SIMD_NEON", RETRO_SIMD_NEON},
        {"RETRO_SIMD_SSE3", RETRO_SIMD_SSE3},
        {"RETRO_SIMD_SSSE3", RETRO_SIMD_SSSE3},
        {"RETRO_SIMD_MMX", RETRO_SIMD_MMX},
        {"RETRO_SIMD_MMXEXT", RETRO_SIMD_MMXEXT},
        {"RETRO_SIMD_SSE4", RETRO_SIMD_SSE4},
        {"RETRO_SIMD_SSE42", RETRO_SIMD_SSE42},
        {"RETRO_SIMD_AVX2", RETRO_SIMD_AVX2},
        {"RETRO_SIMD_VFPU", RETRO_SIMD_VFPU},
        {"RETRO_SIMD_PS", RETRO_SIMD_PS},
        {"RETRO_SIMD_AES", RETRO_SIMD_AES},
        {"RETRO_SIMD_VFPV3", RETRO_SIMD_VFPV3},
        {"RETRO_SIMD_VFPV4", RETRO_SIMD_VFPV4},
        {"RETRO_SIMD_POPCNT", RETRO_SIMD_POPCNT},
        {"RETRO_SIMD_MOVBE", RETRO_SIMD_MOVBE},
        {"RETRO_SIMD_CMOV", RETRO_SIMD_CMOV},
        {"RETRO_SIMD_ASIMD", RETRO_SIMD_ASIMD},
        {"RETRO_SENSOR_ACCELEROMETER_ENABLE", RETRO_SENSOR_ACCELEROMETER_ENABLE},
        {"RETRO_SENSOR_ACCELEROMETER_DISABLE", RETRO_SENSOR_ACCELEROMETER_DISABLE},
        {"RETRO_SENSOR_GYROSCOPE_ENABLE", RETRO_SENSOR_GYROSCOPE_ENABLE},
        {"RETRO_SENSOR_GYROSCOPE_DISABLE", RETRO_SENSOR_GYROSCOPE_DISABLE},
        {"RETRO_SENSOR_ILLUMINANCE_ENABLE", RETRO_SENSOR_ILLUMINANCE_ENABLE},
        {"RETRO_SENSOR_ILLUMINANCE_DISABLE", RETRO_SENSOR_ILLUMINANCE_DISABLE},
        {"RETRO_SENSOR_DUMMY", RETRO_SENSOR_DUMMY},
        {"RETRO_SENSOR_ACCELEROMETER_X", RETRO_SENSOR_ACCELEROMETER_X},
        {"RETRO_SENSOR_ACCELEROMETER_Y", RETRO_SENSOR_ACCELEROMETER_Y},
        {"RETRO_SENSOR_ACCELEROMETER_Z", RETRO_SENSOR_ACCELEROMETER_Z},
        {"RETRO_SENSOR_GYROSCOPE_X", RETRO_SENSOR_GYROSCOPE_X},
        {"RETRO_SENSOR_GYROSCOPE_Y", RETRO_SENSOR_GYROSCOPE_Y},
        {"RETRO_SENSOR_GYROSCOPE_Z", RETRO_SENSOR_GYROSCOPE_Z},
        {"RETRO_SENSOR_ILLUMINANCE", RETRO_SENSOR_ILLUMINANCE},
        {"RETRO_CAMERA_BUFFER_OPENGL_TEXTURE", RETRO_CAMERA_BUFFER_OPENGL_TEXTURE},
        {"RETRO_CAMERA_BUFFER_RAW_FRAMEBUFFER", RETRO_CAMERA_BUFFER_RAW_FRAMEBUFFER},
        {"RETRO_CAMERA_BUFFER_DUMMY", RETRO_CAMERA_BUFFER_DUMMY},
        {"RETRO_CAMERA_BUFFER_OPENGL_TEXTURE", RETRO_CAMERA_BUFFER_OPENGL_TEXTURE},
        {"RETRO_RUMBLE_STRONG", RETRO_RUMBLE_STRONG},
        {"RETRO_RUMBLE_WEAK", RETRO_RUMBLE_WEAK},
        {"RETRO_RUMBLE_DUMMY", RETRO_RUMBLE_DUMMY},
        {"RETRO_HW_CONTEXT_NONE", RETRO_HW_CONTEXT_NONE},
        {"RETRO_HW_CONTEXT_OPENGL", RETRO_HW_CONTEXT_OPENGL},
        {"RETRO_HW_CONTEXT_OPENGLES2", RETRO_HW_CONTEXT_OPENGLES2},
        {"RETRO_HW_CONTEXT_OPENGL_CORE", RETRO_HW_CONTEXT_OPENGL_CORE},
        {"RETRO_HW_CONTEXT_OPENGLES3", RETRO_HW_CONTEXT_OPENGLES3},
        {"RETRO_HW_CONTEXT_OPENGLES_VERSION", RETRO_HW_CONTEXT_OPENGLES_VERSION},
        {"RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE", RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE},
        {"RETRO_HW_CONTEXT_VULKAN", RETRO_HW_CONTEXT_VULKAN},
        {"RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE", RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE},
        {"RETRO_HW_CONTEXT_DIRECT3D", RETRO_HW_CONTEXT_DIRECT3D},
        {"RETRO_HW_CONTEXT_DUMMY", RETRO_HW_CONTEXT_DUMMY},
        {"RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK", RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK},
        {"RETROK_UNKNOWN", RETROK_UNKNOWN},
        {"RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE", RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE},
        {"RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE", RETRO_ENVIRONMENT_SET_DISK_CONTROL_EXT_INTERFACE},
        {"RETRO_PIXEL_FORMAT_0RGB1555", RETRO_PIXEL_FORMAT_0RGB1555},
        {"RETRO_PIXEL_FORMAT_XRGB8888", RETRO_PIXEL_FORMAT_XRGB8888},
        {"RETRO_PIXEL_FORMAT_RGB565", RETRO_PIXEL_FORMAT_RGB565},
        {"RETRO_PIXEL_FORMAT_UNKNOWN", RETRO_PIXEL_FORMAT_UNKNOWN},
        {"RETRO_ENVIRONMENT_GET_VARIABLE", RETRO_ENVIRONMENT_GET_VARIABLE},
        {"RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY", RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY},
        {"RETRO_NUM_CORE_OPTION_VALUES_MAX", RETRO_NUM_CORE_OPTION_VALUES_MAX},
        {"RETRO_ENVIRONMENT_GET_VARIABLE", RETRO_ENVIRONMENT_GET_VARIABLE},
        {"RETRO_NUM_CORE_OPTION_VALUES_MAX", RETRO_NUM_CORE_OPTION_VALUES_MAX},
        {"RETRO_MEMORY_ACCESS_WRITE", RETRO_MEMORY_ACCESS_WRITE},
        {"RETRO_MEMORY_ACCESS_READ", RETRO_MEMORY_ACCESS_READ},
        {"RETRO_MEMORY_TYPE_CACHED", RETRO_MEMORY_TYPE_CACHED},
        {"RETRO_ENVIRONMENT_SET_PIXEL_FORMAT", RETRO_ENVIRONMENT_SET_PIXEL_FORMAT},
        {"RETRO_DEVICE_MASK", RETRO_DEVICE_MASK},
        {"RETRO_DEVICE_JOYPAD", RETRO_DEVICE_JOYPAD},
        {"RETRO_DEVICE_JOYPAD", RETRO_DEVICE_JOYPAD},
        {"RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS", RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS},
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
