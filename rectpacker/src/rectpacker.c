#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#include <lua.h>
#include <lauxlib.h>

#include <stdlib.h>

#define RECTS_MT "Rects"

typedef struct {
    stbrp_rect* rects;
    size_t count;
    size_t allocated;
}
Rects;

static Rects* checkrects(lua_State* const L, int const ndx) {
    Rects* const self = (Rects*)luaL_checkudata(L, ndx, RECTS_MT);
    return self;
}

static int l_appendrects(lua_State* const L) {
    Rects* const self = checkrects(L, 1);
    lua_Integer const w = luaL_checkinteger(L, 2);
    lua_Integer const h = luaL_checkinteger(L, 3);
    lua_Integer const id = luaL_checkinteger(L, 4);

    if (self->count == self->allocated) {
        size_t const newallocated = self->allocated == 0 ? 16 : (self->allocated * 2);
        stbrp_rect* const newrects = (stbrp_rect*)realloc(self->rects, newallocated * sizeof(stbrp_rect));

        if (newrects == NULL) {
            lua_pushnil(L);
            lua_pushliteral(L, "out of memory");
            return 2;
        }

        self->allocated = newallocated;
        self->rects = newrects;
    }

    stbrp_rect* const rect = self->rects + self->count++;

    rect->id = (int)id;
    rect->w = w;
    rect->h = h;
    rect->x = STBRP__MAXVAL;
    rect->y = STBRP__MAXVAL;
    rect->was_packed = 0;

    return 0;
}

static int l_getrects(lua_State* const L) {
    Rects* const self = checkrects(L, 1);
    lua_Integer const ndx = luaL_checkinteger(L, 2) - 1;

    if (ndx < 0 || ndx >= self->count) {
        lua_pushnil(L);
        lua_pushfstring(L, "invalid index %I into the array, count is %I", ndx + 1, (lua_Integer)self->count);
        return 2;
    }

    lua_pushboolean(L, self->rects[ndx].was_packed != 0);
    lua_pushinteger(L, self->rects[ndx].x);
    lua_pushinteger(L, self->rects[ndx].y);
    lua_pushinteger(L, self->rects[ndx].w);
    lua_pushinteger(L, self->rects[ndx].h);
    lua_pushinteger(L, self->rects[ndx].id);
    return 6;
}

static int l_resetrects(lua_State* const L) {
    Rects* const self = checkrects(L, 1);
    self->count = 0;
    return 0;
}

static int l_lenrects(lua_State* const L) {
    Rects* const self = checkrects(L, 1);
    lua_pushinteger(L, self->count);
    return 1;
}

static int l_tostringrects(lua_State* const L) {
    Rects* const self = checkrects(L, 1);
    lua_pushfstring(L, RECTS_MT "(%I/%I)", (lua_Integer)self->count, (lua_Integer)self->allocated);
    return 1;
}

static int l_gcrects(lua_State* const L) {
    Rects* const self = checkrects(L, 1);
    free((void*)self->rects);
    return 0;
}

static int l_newrects(lua_State* const L) {
    lua_Integer const initialcapacity = luaL_optinteger(L, 1, 0);
    Rects* const self = (Rects*)lua_newuserdata(L, sizeof(*self));

    self->rects = NULL;
    self->count = 0;
    self->allocated = 0;

    if (initialcapacity > 0) {
        self->allocated = (size_t)initialcapacity;
        self->rects = (stbrp_rect*)realloc(self->rects, self->allocated * sizeof(stbrp_rect));

        if (self->rects == NULL) {
            lua_pushnil(L);
            lua_pushliteral(L, "out of memory");
            return 2;
        }
    }

    if (luaL_newmetatable(L, RECTS_MT)) {
        static luaL_Reg const methods[] = {
            {"append", l_appendrects},
            {"get", l_getrects},
            {"reset", l_resetrects},
            {NULL, NULL}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_lenrects);
        lua_setfield(L, -2, "__len");

        lua_pushcfunction(L, l_tostringrects);
        lua_setfield(L, -2, "__tostring");

        lua_pushcfunction(L, l_gcrects);
        lua_setfield(L, -2, "__gc");
    }

    lua_setmetatable(L, -2);
    return 1;
}

#define RECTPACKER_MT "RectPacker"

typedef struct {
    stbrp_context context;
    int width;
    int height;
    int numnodes;
    stbrp_node nodes[1];
}
RectPacker;

static RectPacker* check(lua_State* const L, int const ndx) {
    RectPacker* const self = (RectPacker*)luaL_checkudata(L, ndx, RECTPACKER_MT);
    return self;
}

static int l_pack(lua_State* const L) {
    RectPacker* const self = check(L, 1);

    if (lua_isinteger(L, 2)) {
        lua_Integer const w = luaL_checkinteger(L, 2);
        lua_Integer const h = luaL_checkinteger(L, 3);

        stbrp_rect rect = {0, (int)w, (int)h, 0, 0, 0};
        int const res = stbrp_pack_rects(&self->context, &rect, 1);


        if (res != 0) {
            lua_pushboolean(L, 1);
            lua_pushinteger(L, rect.x);
            lua_pushinteger(L, rect.y);
            return 3;
        }
        else {
            lua_pushboolean(L, 0);
            return 1;
        }
    }
    else {
        Rects* const rects = checkrects(L, 2);
        int const res = stbrp_pack_rects(&self->context, rects->rects, (int)rects->count);
        lua_pushboolean(L, res != 0);
        return 1;
    }
}

static int l_reset(lua_State* const L) {
    RectPacker* const self = check(L, 1);
    stbrp_init_target(&self->context, self->width, self->height, self->nodes, self->numnodes);
    return 0;
}

static int l_new(lua_State* const L) {
    lua_Integer const width = luaL_checkinteger(L, 1);
    lua_Integer const height = luaL_checkinteger(L, 2);
    lua_Integer const numnodes = luaL_optinteger(L, 3, width);

    if (width <= 0 || height <= 0 || numnodes <= 0) {
        lua_pushnil(L);
        lua_pushfstring(L, "invalid arguments creating the " RECTPACKER_MT);
        return 2;
    }

    size_t const size = sizeof(RectPacker) + sizeof(stbrp_node) * (numnodes - 1);
    RectPacker* const self = (RectPacker*)lua_newuserdata(L, size);

    stbrp_init_target(&self->context, (int)width, (int)height, self->nodes, (int)numnodes);
    self->width = (int)width;
    self->height = (int)height;
    self->numnodes = (int)numnodes;

    if (luaL_newmetatable(L, RECTPACKER_MT)) {
        static luaL_Reg const methods[] = {
            {"pack", l_pack},
            {"reset", l_reset},
            {NULL, NULL}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
    }

    lua_setmetatable(L, -2);
    return 1;
}

LUAMOD_API int luaopen_rectpacker(lua_State* const L) {
    static const luaL_Reg functions[] = {
        {"newRects", l_newrects},
        {"newPacker", l_new},
        {NULL, NULL}
    };

    luaL_newlib(L, functions);
    return 1;
}
