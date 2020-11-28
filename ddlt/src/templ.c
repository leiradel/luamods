#include <string.h>
#include <stdio.h>

#include <lua.h>
#include <lauxlib.h>

static void adduint(luaL_Buffer* const buffer, unsigned const uint) {
    char string[32];
    int const length = snprintf(string, sizeof(string), "%u", uint);
    luaL_addlstring(buffer, string, length);
}

int l_newTemplate(lua_State* const L) {
    size_t length;
    char const* current = luaL_checklstring(L, 1, &length);

    char const* const end = current + length;
    unsigned line = 1;

    size_t open_length;
    char const* const open_tag = luaL_checklstring(L, 2, &open_length);

    size_t close_length;
    char const* const close_tag = luaL_checklstring(L, 3, &close_length);

    char const* const chunkname = luaL_optstring(L, 4, "template");
  
    luaL_Buffer code;
    luaL_buffinit(L, &code);
    luaL_addstring(&code, "return function(args, emit); ");
  
    for (;;) {
        char const* start = current;
        unsigned curr_line = line;
    
        for (;;) {
            char const* const eol = strchr(start, '\n');
            start = strstr(start, open_tag);

            if (start == NULL) {
                break;
            }

            if (eol != NULL && eol < start) {
                start = eol + 1;
                line++;
                continue;
            }

            if (start[open_length] == '=' || start[open_length] == '!') {
                break;
            }

            start++;
        }
    
        if (start == NULL) {
            luaL_addstring(&code, "emit(");
            adduint(&code, curr_line);
            luaL_addstring(&code, ", [===[");
            luaL_addlstring(&code, current, end - current);
            luaL_addstring(&code, "]===])");
            break;
        }

        char const* finish = NULL;

        for (;;) {
            finish = strstr(start + open_length + 1, close_tag);
            char const* const eol = strchr(start + open_length + 1, '\n');
      
            if (finish == NULL) {
                finish = end;
                break;
            }

            if (eol != NULL && eol < finish) {
                finish = eol + 1;
                line++;
                continue;
            }

            break;
        }
        
        if (*current == '\n' || *current == '\r') {
            luaL_addstring(&code, "emit(");
            adduint(&code, curr_line);
            curr_line = line;

            luaL_addstring(&code, ", \'\\n\') emit(");
            adduint(&code, curr_line);
            luaL_addstring(&code, ", [===[");
        }
        else {
            luaL_addstring(&code, "emit(");
            adduint(&code, curr_line);
            luaL_addstring(&code, ", [===[");
        }
    
        luaL_addlstring(&code, current, start - current);
        luaL_addstring(&code, "]===]) ");
    
        if (start[open_length] == '=') {
            start += open_length + 1;
            luaL_addstring(&code, "emit(");
            adduint(&code, curr_line);
            luaL_addstring(&code, ", tostring(");
            luaL_addlstring(&code, start, finish - start);
            luaL_addstring(&code, ")) ");
        }
        else {
            start += open_length + 1;
            luaL_addlstring(&code, start, finish - start);
            luaL_addchar(&code, ' ');
        }
    
        current = finish + close_length;
    }
  
    luaL_addstring(&code, " end\n");
    luaL_pushresult(&code);

    size_t source_length;
    char const* const source = lua_tolstring(L, -1, &source_length);

    int const load_res = luaL_loadbufferx(L, source, source_length, chunkname, "t");

    if (load_res != LUA_OK) {
        lua_pushnil(L);
        lua_pushvalue(L, -2);
        return 2;
    }

    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        lua_pushnil(L);
        lua_pushvalue(L, -2);
        return 2;
    }

    return 1;
}
