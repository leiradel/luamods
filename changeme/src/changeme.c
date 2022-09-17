/*
MIT License

Copyright (c) 2020 Andre Leiradella

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <lua.h>
#include <lauxlib.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "easing.inl"

/*-----------------------------------------------------------------------------\
| Compile-time configuration                                                   |
\-----------------------------------------------------------------------------*/

#ifndef CHANGEME_MAX_PAIRS
#define CHANGEME_MAX_PAIRS 4
#endif

#ifndef CHANGEME_INITIAL_ARRAY_SIZE
#define CHANGEME_INITIAL_ARRAY_SIZE 32
#endif

/*-----------------------------------------------------------------------------\
| Implementation, don't bother                                                 |
\-----------------------------------------------------------------------------*/

#define CHANGE_MT "change"
#define INVALID_INDEX SIZE_MAX

/* States and flags */
enum {
    /* Free for use, will not be processed */
    STATE_UNUSED = 0,

    /* Awaiting for start */
    STATE_PAUSED = 1,

    /* Active */
    STATE_RUNNING = 2,

    /* A simple change that interpolate values over time */
    TYPE_SIMPLE = 0,

    /* A group change that runs child changes in sequence */
    TYPE_GROUP = 1,
};

/* State for simple changes */
typedef struct {
    /* The initial values for the fields */
    lua_Number initial_values[CHANGEME_MAX_PAIRS];

    /* The amount of change for the duration period */
    lua_Number delta_values[CHANGEME_MAX_PAIRS];

    // The number of values used in the array
    size_t num_pairs;

    /**
     * The reference to a callback function that will be called when the
     * change finished
     */
    int callback_ref;
}
Simple;

/* The full change state, fields sorted by alignment descending */
typedef struct {
    /* The duration of the change in seconds */
    lua_Number duration;

    /* The elapsed time since the change was started */
    lua_Number elapsed_time;

    union {
        /* Fields for simple changes */
        Simple simple;

        /* The index of the next free change in the free list */
        size_t next_free;
    }
    u;

    /* In what state this change is, and its flags */
    struct {
        /* The current state of the change */
        unsigned state: 2;

        /* The type of the change */
        unsigned type: 1;

        /* The ease function to use with the change */
        unsigned ease_func: 6;

        /* Should the change repeat after the end? */ 
        unsigned repeat: 1;

        /* Should the change only run the callback once at the end? */
        unsigned alarm: 1;
    }
    state;

    /* The change tag to differentiate living and dead changes apart */
    unsigned tag;

    /* The reference to the change object */
    int my_ref;
}
Change;

/* The userdata that represents a change in Lua space */
typedef struct {
    /* The index of the change in the s_changes array */
    size_t change_index;

    /**
     * Tags are used to prevent long living references to dead changes in Lua
     * land from locking an entry in s_changes until they are garbage
     * collected.
     * 
     * When a change is allocated from s_changes, the tag below is set to the
     * tag in s_changes. When a change finishes and the entry in s_changes is
     * made available for reuse, the tag there is incremented.
     * 
     * When we receive a reference to a change from Lua, the tags are
     * compared. If they're equal, it's a valid reference. If they're
     * different, the change has already finished and the reference is treated
     * as a valid reference to a dead change.
     */
    unsigned change_tag;
}
Userdata;

/* The current array of changes */
static Change* s_changes;

/* The number of entries used in the array */
static size_t s_num_changes;

/* The total number of elements in the array */
static size_t s_reserved_changes;

/* The head of the list of free changes */
static size_t s_free;

/* The type of an ease function in easing.inl */
typedef lua_Number(*EaseFunc)(lua_Number);

/* An array with all the ease functions from easing.inl */
static const EaseFunc s_ease_funcs[] = {
    LinearInterpolation,
    BackEaseIn,
    BackEaseOut,
    BackEaseInOut,
    BounceEaseIn,
    BounceEaseOut,
    BounceEaseInOut,
    CircularEaseIn,
    CircularEaseOut,
    CircularEaseInOut,
    CubicEaseIn,
    CubicEaseOut,
    CubicEaseInOut,
    ElasticEaseIn,
    ElasticEaseOut,
    ElasticEaseInOut,
    ExponentialEaseIn,
    ExponentialEaseOut,
    ExponentialEaseInOut,
    QuadraticEaseIn,
    QuadraticEaseOut,
    QuadraticEaseInOut,
    QuarticEaseIn,
    QuarticEaseOut,
    QuarticEaseInOut,
    QuinticEaseIn,
    QuinticEaseOut,
    QuinticEaseInOut,
    SineEaseIn,
    SineEaseOut,
    SineEaseInOut,
};

/* Allocates a new change */
static size_t alloc_change(void) {
    /* If the free list is not empty... */
    if (s_free != INVALID_INDEX) {
        /* Get a change from the head of the free list */
        size_t const index = s_free;
        s_free = s_changes[index].u.next_free;
        return index;
    }

    /* If the array is full... */
    if (s_num_changes == s_reserved_changes) {
        /* ... grow it */
        size_t const reserve = s_reserved_changes == 0
                             ? CHANGEME_INITIAL_ARRAY_SIZE
                             : s_reserved_changes * 2;

        size_t const size = sizeof(Change) * reserve;
        Change* const changes = (Change*)realloc(s_changes, size);

        if (changes == NULL) {
            return INVALID_INDEX;
        }

        s_changes = changes;
        s_reserved_changes = reserve;
    }

    /* Grab an unused element in the array */
    size_t const index = s_num_changes++;

    /* Set the tag to 0 on freshly allocated changes */
    s_changes[index].tag = 0;

    return index;
}

/* Deallocates a change */
static void free_change(lua_State* const L, size_t const index) {
    Change* const change = s_changes + index;

    /* Increment the tag so that any existing references are considered dead */
    change->tag++;

    /* Put the change in the free list */
    change->u.next_free = s_free;
    s_free = index;

    /* Make sure it won't be processed anymore */
    change->state.state = STATE_UNUSED;

    /* Release the references to the Lua values */
    luaL_unref(L, LUA_REGISTRYINDEX, change->my_ref);
    luaL_unref(L, LUA_REGISTRYINDEX, change->u.simple.callback_ref);
}

/* Checks for a valid change in the Lua stack */
static size_t check_change(lua_State* const L, int ndx) {
    Userdata const* const ud = (Userdata*)luaL_checkudata(L, ndx, CHANGE_MT);

    /* A change is only valid if its tag matches the one from the reference */
    if (s_changes[ud->change_index].tag == ud->change_tag) {
        return ud->change_index;
    }

    return INVALID_INDEX;
}

/* Starts a paused change */
static int l_start(lua_State* const L) {
    size_t const index = check_change(L, 1);

    if (index != INVALID_INDEX) {
        Change* const change = s_changes + index;

        if (change->state.state == STATE_PAUSED) {
            /* Make it run */
            change->state.state = STATE_RUNNING;

            lua_settop(L, 1);
            return 1;
        }

        return luaL_error(L, "change is not paused");
    }

    return luaL_error(L, "change is dead");
}

/* Makes a change repeat after the end */
static int l_repeats(lua_State* const L) {
    size_t const index = check_change(L, 1);

    if (index != INVALID_INDEX) {
        Change* const change = s_changes + index;

        if (change->state.state == STATE_PAUSED) {
            /* Set the repeat flag */
            change->state.repeat = 1;

            lua_settop(L, 1);
            return 1;
        }

        return luaL_error(L, "change is not paused");
    }

    return luaL_error(L, "change is dead");
}

/* Makes the change act as an alarm */
static int l_alarm(lua_State* const L) {
    size_t const index = check_change(L, 1);

    if (index != INVALID_INDEX) {
        Change* const change = s_changes + index;

        if (change->state.state == STATE_PAUSED) {
            /* Set the alarm flag */
            change->state.alarm = 1;

            lua_settop(L, 1);
            return 1;
        }

        return luaL_error(L, "change is not paused");
    }

    return luaL_error(L, "change is dead");
}

/* Kills a running change */
static int l_kill(lua_State* const L) {
    size_t const index = check_change(L, 1);

    if (index != INVALID_INDEX) {
        free_change(L, index);
        return 0;
    }

    return luaL_error(L, "change is already dead");
}

/* Returns the status of a change */
static int l_status(lua_State* const L) {
    size_t const index = check_change(L, 1);
    unsigned const state = index == INVALID_INDEX ? STATE_UNUSED : s_changes[index].state.state;

    switch (state) {
        case STATE_UNUSED: {
            lua_pushliteral(L, "dead");
            return 1;
        }

        case STATE_PAUSED: {
            /* Return "suspended", as a paused Lua coroutine would */
            lua_pushliteral(L, "suspended");
            return 1;
        }

        case STATE_RUNNING: {
            lua_pushliteral(L, "running");
            return 1;
        }
    }

    /* New state added, add it to the above switch */
    return luaL_error(L, "unhandled state in status() method");
}

/* Cleans-up a change collected during a Lua GC cycle */
static int l_gc(lua_State* const L) {
    size_t const index = check_change(L, 1);

    if (index != INVALID_INDEX && s_changes[index].state.state == STATE_PAUSED) {
        /**
         * Only free changes here if they are paused, since there are no
         * references to it left in Lua land and they cannot be started
         * anymore. Other changes that are active will run to completion and
         * be freed inside the update function.
         */
        free_change(L, index);
    }

    return 0;
}

/* Returns an user-friendly string for the change */
static int l_tostring(lua_State* const L) {
    Userdata* const ud = (Userdata*)luaL_checkudata(L, 1, CHANGE_MT);

    char str[64];
    int const len = snprintf(str, sizeof(str), "change(%zu,%u)", ud->change_index, ud->change_tag);

    lua_pushlstring(L, str, len);
    return 1;
}

/* Pushes a change onto the Lua stack */
static int push_change(lua_State* const L, Change* const change) {
    /* Create and initialize the userdata object */
    Userdata* const ud = lua_newuserdata(L, sizeof(*ud));
    ud->change_index = change - s_changes;
    ud->change_tag = change->tag;

    /* Set the userdata's metatable */
    if (luaL_newmetatable(L, "change")) {
        static const luaL_Reg methods[] = {
            {"start",   l_start},
            {"repeats", l_repeats},
            {"alarm",   l_alarm},
            {"kill",    l_kill},
            {"status",  l_status},
            {NULL,      NULL}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");
        
        lua_pushcfunction(L, l_gc);
        lua_setfield(L, -2, "__gc");

        lua_pushcfunction(L, l_tostring);
        lua_setfield(L, -2, "__tostring");
    }

    lua_setmetatable(L, -2);

    /* Make a reference to itself */
    lua_pushvalue(L, -1);
    change->my_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    return 1;
}

/* Creates a change, to == 0 means that target values are relative */
static int create_change(lua_State* const L, int const to) {
    int const top = lua_gettop(L);
    size_t const index = alloc_change();

    if (index == INVALID_INDEX) {
        return luaL_error(L, "out of memory");
    }

    Change* const change = s_changes + index;

    /* Initialize constant values */
    change->state.state = STATE_PAUSED;
    change->state.type = TYPE_SIMPLE;
    change->state.repeat = 0;
    change->state.alarm = 0;
    change->elapsed_time = 0;

    /* Check the change duration */
    if (!lua_isnumber(L, 1)) {
        free_change(L, index);
        return luaL_error(L, "duration is not a number");
    }

    change->duration = lua_tonumber(L, 1);

    /* Get the ease function */
    {
        if (!lua_isinteger(L, 2)) {
            free_change(L, index);
            return luaL_error(L, "invalid ease function");
        }

        lua_Integer const ease_func = lua_tointeger(L, 2);

        if (ease_func < 0 || (lua_Unsigned)ease_func >= sizeof(s_ease_funcs) / sizeof(s_ease_funcs[0])) {
            free_change(L, index);
            return luaL_error(L, "invalid ease function %I", ease_func);
        }

        change->state.ease_func = ease_func;
    }

    /* Iterate over pairs of <initial value, target value> */
    {
        /* Keep track of the number of value pairs since there's a limit */
        size_t const max_pairs = sizeof(change->u.simple.delta_values) / sizeof(change->u.simple.delta_values[0]);
        size_t num_pairs = 0;

        /* Walk the stack */
        for (size_t i = 3; i < (size_t)(top - 1); i += 2, num_pairs++) {
            if (!lua_isnumber(L, i) || !lua_isnumber(L, i + 1)) {
                free_change(L, index);
                return luaL_error(L, "initial and target values must be numbers");
            }

            if (num_pairs == max_pairs) {
                free_change(L, index);
                return luaL_error(L, "too many <initial, target> value pairs, maximum is %zu", max_pairs);
            }

            change->u.simple.initial_values[num_pairs] = lua_tonumber(L, i);
            lua_Number by = lua_tonumber(L, i + 1);

            if (to) {
                /**
                 * If the target value is absolute, evaluate the change amount
                 */
                by -= change->u.simple.initial_values[num_pairs];
            }

            change->u.simple.delta_values[num_pairs] = by;
        }

        change->u.simple.num_pairs = num_pairs;
    }

    /* Make a reference to the callback to call when the change ends */
    change->u.simple.callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    return push_change(L, change);
}

/* Create a change with absolute target values */
static int l_to(lua_State* const L) {
    return create_change(L, 1);
}

/* Create a change with relative target values */
static int l_by(lua_State* const L) {
    return create_change(L, 0);
}

static void update_change(lua_State* const L, size_t const index, lua_Number const dt) {
    Change* const change = s_changes + index;

    change->elapsed_time += dt;
    int const finished = change->elapsed_time >= change->duration;

    if (!finished && change->state.alarm) {
        /* Changes with this flag only update when they finish */
        return;
    }

    /* Apply the ease function */
    lua_Number const in = finished ? 1 : change->elapsed_time / change->duration;
    lua_Number const out = s_ease_funcs[change->state.ease_func](in);

    /* Update the fields in the object */
    lua_rawgeti(L, LUA_REGISTRYINDEX, change->u.simple.callback_ref);
    lua_rawgeti(L, LUA_REGISTRYINDEX, change->my_ref);

    size_t const num_pairs = change->u.simple.num_pairs;

    for (size_t i = 0; i < num_pairs; i++) {
        lua_Number const current = change->u.simple.initial_values[i] +
                                   change->u.simple.delta_values[i] * out;

        lua_pushnumber(L, current);
    }

    lua_call(L, (int)num_pairs + 1, 0);

    // s_changes may have been reallocated here during the callback execution
    Change* const change2 = s_changes + index;

    // Check if the change hasn't been killed in the callback
    if (change2->state.state == STATE_UNUSED) {
        return;
    }

    /* Repeat the change, or free it if it's finished */
    if (finished) {
        if (change2->state.repeat) {
            change2->elapsed_time -= change2->duration;
        }
        else {
            free_change(L, index);
        }
    }
}

/**
 * Update active changes, must be called regularly with the elapsed time since
 * the last call
 */
static int l_update(lua_State* const L) {
    lua_Number const dt = luaL_checknumber(L, 1);

    size_t i;
    size_t const count = s_num_changes;

    for (i = 0; i < count; i++) {
        Change* const change = s_changes + i;

        if (change->state.state != STATE_RUNNING) {
            /* Only process active changes */
            continue;
        }

        int const type = change->state.type;

        switch (type) {
            case TYPE_SIMPLE: update_change(L, i, dt); break;
            default: return luaL_error(L, "invalid change type: %d", type);
        }
    }

    return 0;
}

LUALIB_API int luaopen_changeme(lua_State* const L) {
    static const luaL_Reg functions[] = {
        {"to",     l_to},
        {"by",     l_by},
        {"update", l_update},
        {NULL,     NULL}
    };

    static char const* const ease_names[] = {
        "LINEAR",
        "BACK_IN",
        "BACK_OUT",
        "BACK_IN_OUT",
        "BOUNCE_IN",
        "BOUNCE_OUT",
        "BOUNCE_IN_OUT",
        "CIRCULAR_IN",
        "CIRCULAR_OUT",
        "CIRCULAT_IN_OUT",
        "CUBIC_IN",
        "CUBIC_OUT",
        "CUBIC_IN_OUT",
        "ELASTIC_IN",
        "ELASTIC_OUT",
        "ELASTIC_IN_OUT",
        "EXPONENTIAL_IN",
        "EXPONENTIAL_OUT",
        "EXPONENTIAL_IN_OUT",
        "QUADRATIC_IN",
        "QUADRATIC_OUT",
        "QUADRATIC_IN_OUT",
        "QUARTIC_IN",
        "QUARTIC_OUT",
        "QUARTIC_IN_OUT",
        "QUINTIC_IN",
        "QUINTIC_OUT",
        "QUINTIC_IN_OUT",
        "SINE_IN",
        "SINE_OUT",
        "SINE_IN_OUT"
    };

    static struct {char const* const name; lua_Integer const value;} const constants[] = {
        {"MAX_PAIRS", CHANGEME_MAX_PAIRS}
    };

    static struct {char const* const name; char const* const value;} const info[] = {
        {"_COPYRIGHT", "Copyright (c) 2020-2022 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "3.0.0"},
        {"_NAME", "changeme"},
        {"_URL", "https://github.com/leiradel/luamods/changeme"},
        {"_DESCRIPTION", "A simple module to interpolate values over time"}
    };

    size_t const functions_count = sizeof(functions) / sizeof(functions[0]) - 1;
    size_t const ease_count = sizeof(ease_names) / sizeof(ease_names[0]);
    size_t const constants_count = sizeof(constants) / sizeof(constants[0]);
    size_t const info_count = sizeof(info) / sizeof(info[0]);

    lua_createtable(L, 0, functions_count + ease_count + constants_count + info_count);
    luaL_setfuncs(L, functions, 0);

    for (size_t i = 0; i < ease_count; i++) {
        lua_pushinteger(L, (lua_Integer)i);
        lua_setfield(L, -2, ease_names[i]);
    }

    for (size_t i = 0; i < constants_count; i++) {
        lua_pushinteger(L, constants[i].value);
        lua_setfield(L, -2, constants[i].name);
    }

    for (size_t i = 0; i < info_count; i++) {
        lua_pushstring(L, info[i].value);
        lua_setfield(L, -2, info[i].name);
    }

    /* Initialize the global variables */
    s_changes = NULL;
    s_num_changes = 0;
    s_reserved_changes = 0;
    s_free = INVALID_INDEX;

    return 1;
}
