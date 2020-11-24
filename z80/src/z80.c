#include <lua.h>
#include <lauxlib.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
/* z80.h config and inclusion */
#define CHIPS_IMPL
#define CHIPS_ASSERT(c)
#include "z80.h"
/*---------------------------------------------------------------------------*/

enum {
    ID_UNKNOWN = 0,
    ID_A,
    ID_AF,
    ID_AF_,
    ID_B,
    ID_BC,
    ID_BC_,
    ID_C,
    ID_D,
    ID_DE,
    ID_DE_,
    ID_E,
    ID_EI_PENDING,
    ID_EXEC,
    ID_F,
    ID_FA,
    ID_FA_,
    ID_H,
    ID_HL,
    ID_HL_,
    ID_I,
    ID_IFF1,
    ID_IFF2,
    ID_IM,
    ID_IX,
    ID_IY,
    ID_L,
    ID_OPDONE,
    ID_PC,
    ID_R,
    ID_RESET,
    ID_SET_A,
    ID_SET_AF,
    ID_SET_AF_,
    ID_SET_B,
    ID_SET_BC,
    ID_SET_BC_,
    ID_SET_C,
    ID_SET_D,
    ID_SET_DE,
    ID_SET_DE_,
    ID_SET_E,
    ID_SET_EI_PENDING,
    ID_SET_F,
    ID_SET_FA,
    ID_SET_FA_,
    ID_SET_H,
    ID_SET_HL,
    ID_SET_HL_,
    ID_SET_I,
    ID_SET_IFF1,
    ID_SET_IFF2,
    ID_SET_IM,
    ID_SET_IX,
    ID_SET_IY,
    ID_SET_L,
    ID_SET_PC,
    ID_SET_R,
    ID_SET_SP,
    ID_SET_WZ,
    ID_SP,
    ID_TRAP_CB,
    ID_TRAP_ID,
    ID_WZ
};

static bool l_checkboolean(lua_State* const L, int const index) {
    luaL_checktype(L, index, LUA_TBOOLEAN);
    return lua_toboolean(L, index) == 0 ? false : true;
}

static uint32_t djb2(char const* str) {
    uint32_t hash = 5381;

    while (*str != 0) {
        hash = hash * 33 + (uint8_t)*str++;
    }

    return hash;
}

static int string_to_id(char const* str) {
    uint32_t const hash = djb2(str);
    char const* key = NULL;
    int value = ID_UNKNOWN;

    switch (hash) {
        case UINT32_C(0x0002b606): key = "a"; value = ID_A; break;
        case UINT32_C(0x0059772c): key = "af"; value = ID_AF; break;
        case UINT32_C(0x0b885d0b): key = "af_"; value = ID_AF_; break;
        case UINT32_C(0x0002b607): key = "b"; value = ID_B; break;
        case UINT32_C(0x0059774a): key = "bc"; value = ID_BC; break;
        case UINT32_C(0x0b8860e9): key = "bc_"; value = ID_BC_; break;
        case UINT32_C(0x0002b608): key = "c"; value = ID_C; break;
        case UINT32_C(0x0002b609): key = "d"; value = ID_D; break;
        case UINT32_C(0x0059778e): key = "de"; value = ID_DE; break;
        case UINT32_C(0x0b8869ad): key = "de_"; value = ID_DE_; break;
        case UINT32_C(0x0002b60a): key = "e"; value = ID_E; break;
        case UINT32_C(0xe26440b7): key = "ei_pending"; value = ID_EI_PENDING; break;
        case UINT32_C(0x7c967daa): key = "exec"; value = ID_EXEC; break;
        case UINT32_C(0x0002b60b): key = "f"; value = ID_F; break;
        case UINT32_C(0x005977cc): key = "fa"; value = ID_FA; break;
        case UINT32_C(0x0b8871ab): key = "fa_"; value = ID_FA_; break;
        case UINT32_C(0x0002b60d): key = "h"; value = ID_H; break;
        case UINT32_C(0x00597819): key = "hl"; value = ID_HL; break;
        case UINT32_C(0x0b887b98): key = "hl_"; value = ID_HL_; break;
        case UINT32_C(0x0002b60e): key = "i"; value = ID_I; break;
        case UINT32_C(0x7c98628b): key = "iff1"; value = ID_IFF1; break;
        case UINT32_C(0x7c98628c): key = "iff2"; value = ID_IFF2; break;
        case UINT32_C(0x0059783b): key = "im"; value = ID_IM; break;
        case UINT32_C(0x00597846): key = "ix"; value = ID_IX; break;
        case UINT32_C(0x00597847): key = "iy"; value = ID_IY; break;
        case UINT32_C(0x0002b611): key = "l"; value = ID_L; break;
        case UINT32_C(0x12ef17aa): key = "opdone"; value = ID_OPDONE; break;
        case UINT32_C(0x00597918): key = "pc"; value = ID_PC; break;
        case UINT32_C(0x0002b617): key = "r"; value = ID_R; break;
        case UINT32_C(0x10474288): key = "reset"; value = ID_RESET; break;
        case UINT32_C(0x10595e71): key = "set_a"; value = ID_SET_A; break;
        case UINT32_C(0x1b852cf7): key = "set_af"; value = ID_SET_AF; break;
        case UINT32_C(0x8c2acc36): key = "set_af_"; value = ID_SET_AF_; break;
        case UINT32_C(0x10595e72): key = "set_b"; value = ID_SET_B; break;
        case UINT32_C(0x1b852d15): key = "set_bc"; value = ID_SET_BC; break;
        case UINT32_C(0x8c2ad014): key = "set_bc_"; value = ID_SET_BC_; break;
        case UINT32_C(0x10595e73): key = "set_c"; value = ID_SET_C; break;
        case UINT32_C(0x10595e74): key = "set_d"; value = ID_SET_D; break;
        case UINT32_C(0x1b852d59): key = "set_de"; value = ID_SET_DE; break;
        case UINT32_C(0x8c2ad8d8): key = "set_de_"; value = ID_SET_DE_; break;
        case UINT32_C(0x10595e75): key = "set_e"; value = ID_SET_E; break;
        case UINT32_C(0x53229182): key = "set_ei_pending"; value = ID_SET_EI_PENDING; break;
        case UINT32_C(0x10595e76): key = "set_f"; value = ID_SET_F; break;
        case UINT32_C(0x1b852d97): key = "set_fa"; value = ID_SET_FA; break;
        case UINT32_C(0x8c2ae0d6): key = "set_fa_"; value = ID_SET_FA_; break;
        case UINT32_C(0x10595e78): key = "set_h"; value = ID_SET_H; break;
        case UINT32_C(0x1b852de4): key = "set_hl"; value = ID_SET_HL; break;
        case UINT32_C(0x8c2aeac3): key = "set_hl_"; value = ID_SET_HL_; break;
        case UINT32_C(0x10595e79): key = "set_i"; value = ID_SET_I; break;
        case UINT32_C(0x1188b716): key = "set_iff1"; value = ID_SET_IFF1; break;
        case UINT32_C(0x1188b717): key = "set_iff2"; value = ID_SET_IFF2; break;
        case UINT32_C(0x1b852e06): key = "set_im"; value = ID_SET_IM; break;
        case UINT32_C(0x1b852e11): key = "set_ix"; value = ID_SET_IX; break;
        case UINT32_C(0x1b852e12): key = "set_iy"; value = ID_SET_IY; break;
        case UINT32_C(0x10595e7c): key = "set_l"; value = ID_SET_L; break;
        case UINT32_C(0x1b852ee3): key = "set_pc"; value = ID_SET_PC; break;
        case UINT32_C(0x10595e82): key = "set_r"; value = ID_SET_R; break;
        case UINT32_C(0x1b852f53): key = "set_sp"; value = ID_SET_SP; break;
        case UINT32_C(0x1b852fe1): key = "set_wz"; value = ID_SET_WZ; break;
        case UINT32_C(0x00597988): key = "sp"; value = ID_SP; break;
        case UINT32_C(0xf6299120): key = "trap_cb"; value = ID_TRAP_CB; break;
        case UINT32_C(0xf62991e8): key = "trap_id"; value = ID_TRAP_ID; break;
        case UINT32_C(0x00597a16): key = "wz"; value = ID_WZ; break;

        default: return ID_UNKNOWN;
    }

    return strcmp(str, key) == 0 ? value : ID_UNKNOWN;
}

#define Z80_STATE_MT "Z80State"

typedef struct {
    z80_t z80;
    lua_State* L;
    int tick_ref;
    int trap_cb_ref;
}
Z80State;

static Z80State* z80_check(lua_State* const L, int const index) {
    return (Z80State*)luaL_checkudata(L, index, Z80_STATE_MT);
}

static uint64_t tick(int const num_ticks, uint64_t const pins, void* const user_data) {
    Z80State* const self = (Z80State*)user_data;
    lua_State* const L = self->L;

    lua_rawgeti(L, LUA_REGISTRYINDEX, self->tick_ref);
    lua_pushvalue(L, 1);
    lua_pushinteger(L, num_ticks);
    lua_pushinteger(L, pins);

    lua_call(L, 3, 1);

    if (!lua_isinteger(L, -1)) {
        return luaL_error(L, "'tick' must return an integer");
    }

    lua_Integer const new_pins = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return new_pins;
}

static int trap_cb(uint16_t const pc, uint32_t const ticks, uint64_t const pins, void* const trap_user_data) {
    Z80State* const self = (Z80State*)trap_user_data;
    lua_State* const L = self->L;

    lua_rawgeti(L, LUA_REGISTRYINDEX, self->trap_cb_ref);
    lua_pushvalue(L, 1);
    lua_pushinteger(L, pc);
    lua_pushinteger(L, ticks);
    lua_pushinteger(L, pins);

    lua_call(L, 4, 1);

    if (!lua_isinteger(L, -1)) {
        return luaL_error(L, "'trap_cb' must return an integer");
    }

    lua_Integer const trap_id = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return trap_id;
}

static int l_a(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_a(&self->z80));
    return 1;
}

static int l_af(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_af(&self->z80));
    return 1;
}

static int l_af_(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_af_(&self->z80));
    return 1;
}

static int l_b(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_b(&self->z80));
    return 1;
}

static int l_bc(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_bc(&self->z80));
    return 1;
}

static int l_bc_(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_bc_(&self->z80));
    return 1;
}

static int l_c(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_c(&self->z80));
    return 1;
}

static int l_d(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_d(&self->z80));
    return 1;
}

static int l_de(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_de(&self->z80));
    return 1;
}

static int l_de_(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_de_(&self->z80));
    return 1;
}

static int l_e(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_e(&self->z80));
    return 1;
}

static int l_ei_pending(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushboolean(L, z80_ei_pending(&self->z80));
    return 1;
}

static int l_exec(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const ticks = luaL_checkinteger(L, 2);

    self->L = L;
    uint32_t const executed_ticks = z80_exec(&self->z80, ticks);
    self->L = NULL;

    lua_pushinteger(L, executed_ticks);
    return 1;
}

static int l_f(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_f(&self->z80));
    return 1;
}

static int l_fa(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_fa(&self->z80));
    return 1;
}

static int l_fa_(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_fa_(&self->z80));
    return 1;
}

static int l_h(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_h(&self->z80));
    return 1;
}

static int l_hl(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_hl(&self->z80));
    return 1;
}

static int l_hl_(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_hl_(&self->z80));
    return 1;
}

static int l_i(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_i(&self->z80));
    return 1;
}

static int l_iff1(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushboolean(L, z80_iff1(&self->z80));
    return 1;
}

static int l_iff2(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushboolean(L, z80_iff2(&self->z80));
    return 1;
}

static int l_im(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_im(&self->z80));
    return 1;
}

static int l_ix(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_ix(&self->z80));
    return 1;
}

static int l_iy(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_iy(&self->z80));
    return 1;
}

static int l_l(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_l(&self->z80));
    return 1;
}

static int l_opdone(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushboolean(L, z80_opdone(&self->z80));
    return 1;
}

static int l_pc(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_pc(&self->z80));
    return 1;
}

static int l_r(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_r(&self->z80));
    return 1;
}

static int l_reset(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    z80_reset(&self->z80);
    return 0;
}

static int l_set_a(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_a(&self->z80, value);
    return 1;
}

static int l_set_af(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_af(&self->z80, value);
    return 1;
}

static int l_set_af_(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_af_(&self->z80, value);
    return 1;
}

static int l_set_b(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_b(&self->z80, value);
    return 1;
}

static int l_set_bc(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_bc(&self->z80, value);
    return 1;
}

static int l_set_bc_(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_bc_(&self->z80, value);
    return 1;
}

static int l_set_c(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_c(&self->z80, value);
    return 1;
}

static int l_set_d(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_d(&self->z80, value);
    return 1;
}

static int l_set_de(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_de(&self->z80, value);
    return 1;
}

static int l_set_de_(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_de_(&self->z80, value);
    return 1;
}

static int l_set_e(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_e(&self->z80, value);
    return 1;
}

static int l_set_ei_pending(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    bool const value = l_checkboolean(L, 2);
    z80_set_ei_pending(&self->z80, value);
    return 1;
}

static int l_set_f(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_f(&self->z80, value);
    return 1;
}

static int l_set_fa(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_fa(&self->z80, value);
    return 1;
}

static int l_set_fa_(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_fa_(&self->z80, value);
    return 1;
}

static int l_set_h(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_h(&self->z80, value);
    return 1;
}

static int l_set_hl(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_hl(&self->z80, value);
    return 1;
}

static int l_set_hl_(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_hl_(&self->z80, value);
    return 1;
}

static int l_set_i(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_i(&self->z80, value);
    return 1;
}

static int l_set_iff1(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    bool const value = l_checkboolean(L, 2);
    z80_set_iff1(&self->z80, value);
    return 1;
}

static int l_set_iff2(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = l_checkboolean(L, 2);
    z80_set_iff2(&self->z80, value);
    return 1;
}

static int l_set_im(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_im(&self->z80, value);
    return 1;
}

static int l_set_ix(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_ix(&self->z80, value);
    return 1;
}

static int l_set_iy(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_iy(&self->z80, value);
    return 1;
}

static int l_set_l(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_l(&self->z80, value);
    return 1;
}

static int l_set_pc(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_pc(&self->z80, value);
    return 1;
}

static int l_set_r(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_r(&self->z80, value);
    return 1;
}

static int l_set_sp(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_sp(&self->z80, value);
    return 1;
}

static int l_set_wz(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_Integer const value = luaL_checkinteger(L, 2);
    z80_set_wz(&self->z80, value);
    return 1;
}

static int l_sp(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_sp(&self->z80));
    return 1;
}

static int l_trap_cb(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);

    if (self->trap_cb_ref != LUA_NOREF) {
        luaL_unref(L, LUA_REGISTRYINDEX, self->trap_cb_ref);
        self->trap_cb_ref = LUA_NOREF;

        z80_trap_cb(&self->z80, NULL, NULL);
    }

    if (lua_type(L, 2) != LUA_TNIL) {
        luaL_checktype(L, 2, LUA_TFUNCTION);
        lua_pushvalue(L, 2);
        self->trap_cb_ref = luaL_ref(L, LUA_REGISTRYINDEX);

        z80_trap_cb(&self->z80, trap_cb, (void*)self);
    }

    return 1;
}

static int l_trap_id(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, self->z80.trap_id);
    return 1;
}

static int l_wz(lua_State* const L) {
    Z80State* const self = z80_check(L, 1);
    lua_pushinteger(L, z80_wz(&self->z80));
    return 1;
}

static int l_index(lua_State* const L) {
    static lua_CFunction const functions[] = {
        NULL,
        l_a,
        l_af,
        l_af_,
        l_b,
        l_bc,
        l_bc_,
        l_c,
        l_d,
        l_de,
        l_de_,
        l_e,
        l_ei_pending,
        l_exec,
        l_f,
        l_fa,
        l_fa_,
        l_h,
        l_hl,
        l_hl_,
        l_i,
        l_iff1,
        l_iff2,
        l_im,
        l_ix,
        l_iy,
        l_l,
        l_opdone,
        l_pc,
        l_r,
        l_reset,
        l_set_a,
        l_set_af,
        l_set_af_,
        l_set_b,
        l_set_bc,
        l_set_bc_,
        l_set_c,
        l_set_d,
        l_set_de,
        l_set_de_,
        l_set_e,
        l_set_ei_pending,
        l_set_f,
        l_set_fa,
        l_set_fa_,
        l_set_h,
        l_set_hl,
        l_set_hl_,
        l_set_i,
        l_set_iff1,
        l_set_iff2,
        l_set_im,
        l_set_ix,
        l_set_iy,
        l_set_l,
        l_set_pc,
        l_set_r,
        l_set_sp,
        l_set_wz,
        l_sp,
        l_trap_cb,
        l_trap_id,
        l_wz
    };

    char const* const key = luaL_checkstring(L, 2);
    int const id = string_to_id(key);

    if (id != ID_UNKNOWN) {
        lua_pushcfunction(L, functions[id]);
        return 1;
    }

    return 0;
}

static int l_gc(lua_State* const L) {
    Z80State* const self = (Z80State*)lua_touserdata(L, 1);
    luaL_unref(L, LUA_REGISTRYINDEX, self->tick_ref);

    if (self->trap_cb_ref != LUA_NOREF) {
        luaL_unref(L, LUA_REGISTRYINDEX, self->trap_cb_ref);
    }

    return 0;
}

static int l_init(lua_State* const L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);

    Z80State* const self = (Z80State*)lua_newuserdata(L, sizeof(Z80State));

    z80_init(&self->z80, &(z80_desc_t){
        .tick_cb = tick,
        .user_data = (void*)self
    });

    lua_pushvalue(L, 1);
    self->tick_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    self->trap_cb_ref = LUA_NOREF;
    self->L = NULL;

    if (luaL_newmetatable(L, Z80_STATE_MT) != 0) {
        lua_pushcfunction(L, l_index);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_gc);
        lua_setfield(L, -2, "__gc");
    }

    lua_setmetatable(L, -2);
    return 1;
}

static int l_make_pins(lua_State* const L) {
    lua_Integer const ctrl = luaL_checkinteger(L, 1);
    lua_Integer const addr = luaL_checkinteger(L, 2);
    lua_Integer const data = luaL_checkinteger(L, 3);

    lua_Integer const pins = Z80_MAKE_PINS(ctrl, addr, data);
    lua_pushinteger(L, pins);
    return 1;
}

static int l_get_addr(lua_State* const L) {
    lua_Integer const pins = luaL_checkinteger(L, 1);
    lua_Integer const addr = Z80_GET_ADDR(pins);
    lua_pushinteger(L, addr);
    return 1;
}

static int l_set_addr(lua_State* const L) {
    lua_Integer const pins = luaL_checkinteger(L, 1);
    lua_Integer const addr = luaL_checkinteger(L, 2);

    lua_Integer new_pins = pins;
    Z80_SET_ADDR(new_pins, addr);
    lua_pushinteger(L, new_pins);
    return 1;
}

static int l_get_data(lua_State* const L) {
    lua_Integer const pins = luaL_checkinteger(L, 1);
    lua_Integer const data = Z80_GET_DATA(pins);
    lua_pushinteger(L, data);
    return 1;
}

static int l_set_data(lua_State* const L) {
    lua_Integer const pins = luaL_checkinteger(L, 1);
    lua_Integer const data = luaL_checkinteger(L, 2);

    lua_Integer new_pins = pins;
    Z80_SET_DATA(new_pins, data);
    lua_pushinteger(L, new_pins);
    return 1;
}

static int l_get_wait(lua_State* const L) {
    lua_Integer const pins = luaL_checkinteger(L, 1);
    lua_Integer const wait = Z80_GET_WAIT(pins);
    lua_pushinteger(L, wait);
    return 1;
}

static int l_set_wait(lua_State* const L) {
    lua_Integer const pins = luaL_checkinteger(L, 1);
    lua_Integer const wait = luaL_checkinteger(L, 2);

    lua_Integer new_pins = pins;
    Z80_SET_WAIT(new_pins, wait);
    lua_pushinteger(L, new_pins);
    return 1;
}

static int make_const(lua_State* const L) {
    // Compile the code and leave the chunk at the top of the stack.
    int const load_res = luaL_loadstring(L,
        // This code will return its argument as a constant object only if
        // the access module is available.
        "return function(module)\n"
        "    local found, access = pcall(require, 'access')\n"
        "    return found and access.const(module) or module\n"
        "end\n"
    );

    // Oops.
    if (load_res != LUA_OK) {
        return load_res;
    }

    // Run the chunk and leave the function that it returns at the top of the
    // stack.
    lua_call(L, 0, 1);

    // Move the function to below the object.
    lua_insert(L, -2);

    // Run the Lua code to make the module table constant.
    lua_call(L, 1, 1);

    // All is good.
    return LUA_OK;
}

LUAMOD_API int luaopen_z80(lua_State* const L) {
    static const luaL_Reg functions[] = {
        {"init", l_init},
        {"MAKE_PINS", l_make_pins},
        {"GET_ADDR", l_get_addr},
        {"SET_ADDR", l_set_addr},
        {"GET_DATA", l_get_data},
        {"SET_DATA", l_set_data},
        {"GET_WAIT", l_get_wait},
        {"SET_WAIT", l_set_wait},
        {NULL, NULL}
    };

    static struct {char const* const name; uint64_t const value;} const constants[] = {
        {"A0", Z80_A0},
        {"A1", Z80_A1},
        {"A2", Z80_A2},
        {"A3", Z80_A3},
        {"A4", Z80_A4},
        {"A5", Z80_A5},
        {"A6", Z80_A6},
        {"A7", Z80_A7},
        {"A8", Z80_A8},
        {"A9", Z80_A9},
        {"A10", Z80_A10},
        {"A11", Z80_A11},
        {"A12", Z80_A12},
        {"A13", Z80_A13},
        {"A14", Z80_A14},
        {"A15", Z80_A15},
        {"D0", Z80_D0},
        {"D1", Z80_D1},
        {"D2", Z80_D2},
        {"D3", Z80_D3},
        {"D4", Z80_D4},
        {"D5", Z80_D5},
        {"D6", Z80_D6},
        {"D7", Z80_D7},
        {"M1", Z80_M1},
        {"MREQ", Z80_MREQ},
        {"IORQ", Z80_IORQ},
        {"RD", Z80_RD},
        {"WR", Z80_WR},
        {"RFSH", Z80_RFSH},
        {"CTRL_MASK", Z80_CTRL_MASK},
        {"HALT", Z80_HALT},
        {"INT", Z80_INT},
        {"NMI", Z80_NMI},
        {"WAIT0", Z80_WAIT0},
        {"WAIT1", Z80_WAIT1},
        {"WAIT2", Z80_WAIT2},
        {"WAIT_SHIFT", Z80_WAIT_SHIFT},
        {"WAIT_MASK", Z80_WAIT_MASK},
        {"IEIO", Z80_IEIO},
        {"RETI", Z80_RETI},
        {"PIN_MASK", Z80_PIN_MASK}
    };

    static struct {char const* const name; char const* const value;} const info[] = {
        {"_COPYRIGHT", "Copyright (c) 2020 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "1.0.0"},
        {"_NAME", "z80"},
        {"_URL", "https://github.com/leiradel/luamods/z80"},
        {"_DESCRIPTION", "Bindings for Andre Weissflog's Z80 emulator"}
    };

    size_t const functions_count = sizeof(functions) / sizeof(functions[0]) - 1;
    size_t const constants_count = sizeof(constants) / sizeof(constants[0]);
    size_t const info_count = sizeof(info) / sizeof(info[0]);

    lua_createtable(L, 0, functions_count + constants_count + info_count);
    luaL_setfuncs(L, functions, 0);

    for (size_t i = 0; i < constants_count; i++) {
        lua_pushinteger(L, constants[i].value);
        lua_setfield(L, -2, constants[i].name);
    }

    for (size_t i = 0; i < info_count; i++) {
        lua_pushstring(L, info[i].value);
        lua_setfield(L, -2, info[i].name);
    }

    if (make_const(L) != LUA_OK) {
        return lua_error(L);
    }

    return 1;
}
