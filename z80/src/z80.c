#include <lua.h>
#include <lauxlib.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
/* z80.h and z80dasm.h config and inclusion */
#define CHIPS_IMPL
#define CHIPS_ASSERT(c)
#include "z80.h"
#include "z80dasm.h"
/*---------------------------------------------------------------------------*/

static bool l_checkboolean(lua_State* const L, int const index) {
    luaL_checktype(L, index, LUA_TBOOLEAN);
    return lua_toboolean(L, index) == 0 ? false : true;
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
        static luaL_Reg const methods[] = {
            {"a", l_a},
            {"af", l_af},
            {"af_", l_af_},
            {"b", l_b},
            {"bc", l_bc},
            {"bc_", l_bc_},
            {"c", l_c},
            {"d", l_d},
            {"de", l_de},
            {"de_", l_de_},
            {"e", l_e},
            {"ei_pending", l_ei_pending},
            {"exec", l_exec},
            {"f", l_f},
            {"fa", l_fa},
            {"fa_", l_fa_},
            {"h", l_h},
            {"hl", l_hl},
            {"hl_", l_hl_},
            {"i", l_i},
            {"iff1", l_iff1},
            {"iff2", l_iff2},
            {"im", l_im},
            {"ix", l_ix},
            {"iy", l_iy},
            {"l", l_l},
            {"opdone", l_opdone},
            {"pc", l_pc},
            {"r", l_r},
            {"reset", l_reset},
            {"set_a", l_set_a},
            {"set_af", l_set_af},
            {"set_af_", l_set_af_},
            {"set_b", l_set_b},
            {"set_bc", l_set_bc},
            {"set_bc_", l_set_bc_},
            {"set_c", l_set_c},
            {"set_d", l_set_d},
            {"set_de", l_set_de},
            {"set_de_", l_set_de_},
            {"set_e", l_set_e},
            {"set_ei_pending", l_set_ei_pending},
            {"set_f", l_set_f},
            {"set_fa", l_set_fa},
            {"set_fa_", l_set_fa_},
            {"set_h", l_set_h},
            {"set_hl", l_set_hl},
            {"set_hl_", l_set_hl_},
            {"set_i", l_set_i},
            {"set_iff1", l_set_iff1},
            {"set_iff2", l_set_iff2},
            {"set_im", l_set_im},
            {"set_ix", l_set_ix},
            {"set_iy", l_set_iy},
            {"set_l", l_set_l},
            {"set_pc", l_set_pc},
            {"set_r", l_set_r},
            {"set_sp", l_set_sp},
            {"set_wz", l_set_wz},
            {"sp", l_sp},
            {"trap_cb", l_trap_cb},
            {"trap_id", l_trap_id},
            {"wz", l_wz}
        };

        luaL_newlib(L, methods);
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

typedef struct {
    int input_cb_index;
    lua_State* L;
    luaL_Buffer B;
}
Z80DasmState;

static uint8_t in_cb(void* const user_data) {
    Z80DasmState* const ud = (Z80DasmState*)user_data;

    // Push and call the input callback.
    lua_pushvalue(ud->L, ud->input_cb_index);
    lua_call(ud->L, 0, 1);

    // Oops.
    if (!lua_isinteger(ud->L, -1)) {
        return luaL_error(ud->  L, "'dasm' input callback must return an integer");
    }

    // Get the returned byte, remove it from the stack, and return.
    uint8_t const byte = (uint8_t)lua_tointeger(ud->L, -1);
    lua_pop(ud->L, 1);
    return byte;
}

static void out_cb(char const c, void* const user_data) {
    // Just add the character to the buffer.
    Z80DasmState* const ud = (Z80DasmState*)user_data;
    luaL_addchar(&ud->B, c);
}

static int l_dasm(lua_State* L) {
    Z80DasmState ud;
    ud.input_cb_index = 1;

    // Get an optional address in the first argument.
    uint16_t address = 0;

    if (lua_isinteger(L, 1)) {
        address = (uint16_t)lua_tointeger(L, 1);
        ud.input_cb_index = 2;
    }

    // Make sure we have an input callback.
    luaL_checktype(L, ud.input_cb_index, LUA_TFUNCTION);

    // Prepare the userdata.
    ud.L = L;
    luaL_buffinitsize(L, &ud.B, 32);

    // Disassemble the instruction
    uint16_t const next_address = z80dasm_op(address, in_cb, out_cb, (void*)&ud);

    luaL_pushresult(&ud.B);
    lua_pushinteger(L, next_address);
    return 2;
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
        {"dasm", l_dasm},
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

    return 1;
}
