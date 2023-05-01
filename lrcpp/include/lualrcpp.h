#pragma once

#include <lua.hpp>
#include <lrcpp/Frontend.h>

namespace lrcpp {
    int openf(lua_State* const L);
    int push(lua_State* const L, Frontend* frontend);
    Frontend* check(lua_State* const L, int const ndx);
}
