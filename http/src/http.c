#include <lua.h>
#include <lauxlib.h>

#include <curl/curl.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <stdlib.h>

static CURLM* cm;

typedef struct {
    lua_State* L;
    int cb_ref;
    char error[CURL_ERROR_SIZE];
}
UserData;

static size_t header_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    UserData const* const ud = (UserData*)userdata;
    size_t const bytes = size * nmemb;
    size_t length = bytes;

    while (length != 0) {
        if (ptr[length - 1] != '\n' && ptr[length - 1] != '\r') {
            break;
        }

        length--;
    }

    if (length != 0) {
        lua_rawgeti(ud->L, LUA_REGISTRYINDEX, ud->cb_ref);
        lua_pushliteral(ud->L, "header");
        lua_pushlstring(ud->L, ptr, length);
        lua_call(ud->L, 2, 0);
    }

    return bytes;
}

static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
    UserData const* const ud = (UserData*)userdata;
    size_t const bytes = size * nmemb;

    lua_rawgeti(ud->L, LUA_REGISTRYINDEX, ud->cb_ref);
    lua_pushliteral(ud->L, "data");
    lua_pushlstring(ud->L, ptr, bytes);
    lua_call(ud->L, 2, 1);

    int const abort = lua_toboolean(ud->L, -1);
    lua_pop(ud->L, 1);

    return abort ? 0 : bytes;
}

static int l_get(lua_State* const L) {
    char const* const url = luaL_checkstring(L, 1);

    lua_pushvalue(L, 2);

    UserData* ud = (UserData*)malloc(sizeof(*ud));

    if (ud == NULL) {
        return luaL_error(L, "out of memory");
    }

    CURL* handle = curl_easy_init();

    if (handle == NULL) {
        return luaL_error(L, "error creating easy handle");
    }

    CURLcode const res1 = curl_easy_setopt(handle, CURLOPT_URL, url);

    if (res1 != CURLE_OK) {
        return luaL_error(L, "%s", curl_easy_strerror(res1));
    }

    // Those will always succeed
    curl_easy_setopt(handle, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, header_cb);
    curl_easy_setopt(handle, CURLOPT_HEADERDATA, ud);
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, ud);
    curl_easy_setopt(handle, CURLOPT_PRIVATE, ud);
    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, ud->error);

    CURLMcode const res2 = curl_multi_add_handle(cm, handle);

    if (res2 != CURLM_OK) {
        return luaL_error(L, "%s", curl_multi_strerror(res2));
    }

    ud->cb_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    ud->L = L;

    return 0;
}

static int l_tick(lua_State* const L) {
    int still_alive = 0;
    curl_multi_perform(cm, &still_alive);

    for (;;) {
        int msgs_left = 0;
        CURLMsg* const msg = curl_multi_info_read(cm, &msgs_left);

        if (msg == NULL) {
            break;
        }

        CURL* const handle = msg->easy_handle;
        char* private = NULL;
        curl_easy_getinfo(handle, CURLINFO_PRIVATE, &private);
        UserData const* const ud = (UserData*)private;

        lua_rawgeti(L, LUA_REGISTRYINDEX, ud->cb_ref);

        if (msg->msg == CURLMSG_DONE) {
            if (msg->data.result == CURLE_OK) {
                lua_pushliteral(L, "end");
                lua_call(L, 1, 0);
            }
            else {
                lua_pushliteral(L, "error");
                lua_pushfstring(L, "%s", curl_easy_strerror(msg->data.result));
                lua_call(L, 2, 0);
            }
        }
        else {
            lua_pushliteral(L, "error");
            lua_pushfstring(L, "(CURLMSG)%d", msg->msg);
            lua_call(L, 2, 0);
        }

        luaL_unref(L, LUA_REGISTRYINDEX, ud->cb_ref);
        free((void*)ud);

        curl_multi_remove_handle(cm, handle);
        curl_easy_cleanup(handle);
    }

    return 0;
}

static int l_cleanup(lua_State* const L) {
    curl_multi_cleanup(cm);
    curl_global_cleanup();
    return 0;
}

LUAMOD_API int luaopen_http(lua_State* const L) {
    static const luaL_Reg functions[] = {
        {"get", l_get},
        {"tick", l_tick},
        {NULL, NULL}
    };

    static struct {char const* const name; char const* const value;} const info[] = {
        {"_COPYRIGHT", "Copyright (c) 2022 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "1.0.0"},
        {"_NAME", "http"},
        {"_URL", "https://github.com/leiradel/luamods/http"},
        {"_DESCRIPTION", "A module that performs non-blocking HTTP requests"}
    };

    CURLcode const res = curl_global_init(CURL_GLOBAL_ALL);

    if (res != CURLE_OK) {
        return luaL_error(L, "%s", curl_easy_strerror(res));
    }

    cm = curl_multi_init();

    if (cm == NULL) {
        curl_global_cleanup();
        return luaL_error(L, "error creating the global multi handle");
    }

    size_t const functions_count = sizeof(functions) / sizeof(functions[0]) - 1;
    size_t const info_count = sizeof(info) / sizeof(info[0]);

    lua_createtable(L, 0, functions_count + info_count);
    luaL_setfuncs(L, functions, 0);

    for (size_t i = 0; i < info_count; i++) {
        lua_pushstring(L, info[i].value);
        lua_setfield(L, -2, info[i].name);
    }

    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, l_cleanup);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);

    return 1;
}
