#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include <lua.h>
#include <lauxlib.h>

#ifdef WIN32
#include "realpath.c"
#define PATH_SEPARATOR '\\'
#else
#define _MAX_PATH PATH_MAX
#define PATH_SEPARATOR '/'
#endif

int l_realpath(lua_State* const L) {
    char buffer[_MAX_PATH];

    char const* const path = luaL_checkstring(L, 1);
    char* resolved = realpath(path, buffer);
  
    if (resolved != NULL) {
         while (*resolved != 0) {
            if (*resolved == '\\' ) {
                *resolved = '/';
            }
      
            resolved++;
        }
    
        lua_pushlstring(L, buffer, resolved - buffer);
        return 1;
    }
  
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    return 2;
}

int l_split(lua_State* const L) {
    size_t length;
    char const* const path = luaL_checklstring(L, 1, &length);
    char const* ext = path + length;
  
    while (ext > path && *ext != '.' && *ext != '/' && *ext != '\\') {
        ext--;
    }
  
    char const* name = ext;
  
    if (*ext != '.') {
        ext = NULL;
    }
  
    while (name > path && *name != '/' && *name != '\\') {
        name--;
    }

    if (*name == '/' || *name == '\\') {
        name++;
    }

    if (name - path - 1 > 0) {
        lua_pushlstring(L, path, name - path - 1);
    }
    else {
        lua_pushnil(L);
    }
  
    if (ext != NULL) {
        lua_pushlstring(L, name, ext - name);
        lua_pushstring(L, ext + 1);
    }
    else {
        lua_pushstring(L, name);
        lua_pushnil(L);
    }
  
    return 3;
}

int l_join(lua_State* const L) {
    size_t path_length;
    char const* const path = lua_tolstring(L, 1, &path_length);

    size_t name_length;
    char const* const name = lua_tolstring(L, 2, &name_length);

    size_t ext_length;
    char const* const ext  = lua_tolstring(L, 3, &ext_length);

    luaL_Buffer join;
    luaL_buffinit(L, &join);

    if (path != NULL) {
        luaL_addlstring(&join, path, path_length);
        luaL_addchar(&join, PATH_SEPARATOR);
    }

    if (name != NULL) {
        luaL_addlstring(&join, name, name_length);
    }

    if (ext != NULL) {
        luaL_addchar(&join, '.');
        luaL_addlstring(&join, ext, ext_length);
    }

    luaL_pushresult(&join);
    return 1;
}

int l_scandir(lua_State* const L) {
    char const* const name = luaL_checkstring(L, 1);
    DIR* const dir = opendir( name );

    if (dir != NULL) {
        lua_createtable(L, 0, 0);

        struct dirent const* entry = NULL;
        int ndx = 1;

        while ((entry = readdir(dir)) != NULL) {
            lua_pushfstring(L, "%s/%s", name, entry->d_name);
            lua_rawseti(L, -2, ndx++);
        }
    
        closedir(dir);
        return 1;
    }
  
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    return 2;
}

static void pushtime(lua_State* const L, time_t const* const time) {
    struct tm const* const tm = gmtime(time);

    char buf[40];

    int const written = sprintf(
        buf,
        "%04d-%02d-%02dT%02d:%02d:%02dZ",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec
    );

    lua_pushlstring(L, buf, written);
}

int l_stat(lua_State* const L) {
    static struct {unsigned const flag; char const* const name;} const modes[] = {
#ifdef S_IFSOCK 
        {S_IFSOCK, "sock"},
#endif
#ifdef S_IFLNK 
        {S_IFLNK, "link" },
#endif
        {S_IFREG, "file" },
        {S_IFBLK, "block" },
        {S_IFDIR, "dir" },
        {S_IFCHR, "char" },
        {S_IFIFO, "fifo" },
    };
  
    char const* const name = luaL_checkstring(L, 1);
    struct stat buf;
  
    if (stat(name, &buf) == 0) {
        if (lua_istable(L, 1)) {
            lua_pushvalue(L, 1);
        }
        else {
            lua_createtable(L, 0, 4 + sizeof(modes) / sizeof(modes[0]));
        }
    
        lua_pushinteger(L, buf.st_size);
        lua_setfield(L, -2, "size");
    
        pushtime(L, &buf.st_atime);
        lua_setfield(L, -2, "atime");
    
        pushtime(L, &buf.st_mtime);
        lua_setfield(L, -2, "mtime");
    
        pushtime(L, &buf.st_ctime);
        lua_setfield(L, -2, "ctime");
    
        for (size_t i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
            lua_pushboolean(L, (buf.st_mode & S_IFMT) == modes[i].flag);
            lua_setfield(L, -2, modes[i].name);
        }
    
        return 1;
    }
  
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    return 2;
}
