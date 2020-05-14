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

#ifndef CHANGEME_FIELD_NAMES_SIZE
#define CHANGEME_FIELD_NAMES_SIZE 24
#endif

#ifndef CHANGEME_MAX_FIELDS
#define CHANGEME_MAX_FIELDS 4
#endif

#ifndef CHANGEME_INITIAL_ARRAY_SIZE
#define CHANGEME_INITIAL_ARRAY_SIZE 32
#endif

/*-----------------------------------------------------------------------------\
| Implementation, don't bother                                                 |
\-----------------------------------------------------------------------------*/

/* States and flags */
typedef enum {
    /* Free for use, will not be processed */
    STATE_UNUSED,

    /* Awaiting for start */
    STATE_PAUSED,

    /* Active */
    STATE_RUNNING,

    /* Change has finished */
    STATE_DEAD,

    /* Mask to get the state */
    STATE_MASK = 0x0000ff,

    /* Start the change in pause mode */
    FLAG_PAUSED = 1 << 8,

    /* Repeat the change when it ends */
    FLAG_REPEAT = 1 << 9,

    /* Affect the fields only once at the end of the duration period */
    FLAG_AFTER = 1 << 10,

    /* The change has been released in Lua land */
    FLAG_COLLECTED = 1 << 11,

    /* Mask to get the ease function */
    EASE_MASK = 0xff0000
}
state_t;

/* The type of an ease function in easing.inl */
typedef lua_Number(*ease_func_t)(lua_Number);

/* The full change state, fields sorted by alignment descending */
typedef struct change_t change_t;
struct change_t {
    union {
        /* The ease function to use with this change */
        ease_func_t ease_func;

        /* The pointer to the next free change in the free list */
        change_t* next_free;
    }
    u;

    /* The initial values for the fields */
    lua_Number initial_values[CHANGEME_MAX_FIELDS];

    /* The amount of change for the duration period */
    lua_Number delta_values[CHANGEME_MAX_FIELDS];

    /* The duration of the change in seconds */
    lua_Number duration;

    /* The elapsed time since the change was started */
    lua_Number elapsed_time;

    /* In what state this change is, and its flags */
    state_t state;

    /* The reference to the object that keeps the fields */
    int table_ref;

    /**
     * The reference to a callback function that will be called when the
     * change finished, if provided
     */
    int callback_ref;

    /**
     * The names of the fields in the object, each name ends with 0, the list
     * also ends with a 0 (meaning that the last name is followed by two
     * zeroes)
     */
    char field_names[CHANGEME_FIELD_NAMES_SIZE];
};

/* The userdata that represents a change in Lua space */
typedef struct {
    /* The index of the change in the s_changes array */
    size_t change_index;
}
userdata_t;

/* The current array of changes */
static change_t* s_changes;

/* The number of entries used in the array */
static size_t s_num_changes;

/* The total number of elements in the array */
static size_t s_reserved_changes;

/* The head of the list of free changes */
static change_t* s_free;

/* An array with all the ease functions from easing.inl */
static const ease_func_t s_ease_funcs[] = {
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
static change_t* alloc(void) {
    /* If the free list is not empty... */
    if (s_free != NULL) {
        /* Get a change from the head of the free list */
        change_t* const change = s_free;
        s_free = change->u.next_free;
        return change;
    }

    /* If the array is full... */
    if (s_num_changes == s_reserved_changes) {
        /* ... grow it */
        size_t const reserve = s_reserved_changes == 0
                             ? CHANGEME_INITIAL_ARRAY_SIZE
                             : s_reserved_changes * 2;

        size_t const size = sizeof(change_t) * reserve;
        change_t* const changes = (change_t*)realloc(s_changes, size);

        if (changes == NULL) {
            return NULL;
        }

        s_changes = changes;
        s_reserved_changes = reserve;
    }

    /* Grab an unused element in the array */
    change_t* const change = s_changes + s_num_changes++;
    return change;
}

/* Deallocates a change */
static void free_change(lua_State* const L, change_t* const change) {
    /* Put the change in the free list */
    change->u.next_free = s_free;
    s_free = change;

    /* Make sure it won't be processed anymore */
    change->state = STATE_UNUSED;

    /* Release the references to the Lua values */
    luaL_unref(L, LUA_REGISTRYINDEX, change->table_ref);

    if (change->callback_ref != LUA_NOREF) {
        luaL_unref(L, LUA_REGISTRYINDEX, change->callback_ref);
    }
}

/* Checks for a valid change in the Lua stack */
static change_t* check(lua_State* const L, int index) {
    /* Get the change from the array according to the userdata */
    userdata_t* const ud = (userdata_t*)luaL_checkudata(L, 1, "change");
    return s_changes + ud->change_index;
}

/* Starts a paused change */
static int start(lua_State* const L) {
    change_t* const change = check(L, 1);

    if ((change->state & STATE_MASK) == STATE_PAUSED) {
        /* Make it run */
        change->state &= ~STATE_MASK;
        change->state |= STATE_RUNNING;
        return 0;
    }
    else {
        lua_pushnil(L);
        lua_pushliteral(L, "change is not paused");
        return 2;
    }
}

/* Kills a running change */
static int kill(lua_State* const L) {
    change_t* const change = check(L, 1);
    change->state = STATE_DEAD;
    return 0;
}

/* Returns the status of a change */
static int status(lua_State* const L) {
    change_t* const change = check(L, 1);

    switch (change->state & STATE_MASK) {
        case STATE_UNUSED: /* -> Should never happen */
        case STATE_DEAD: {
            lua_pushliteral(L, "dead");
            return 1;
        }

        case STATE_PAUSED: {
            /* Returned "suspended", as a paused Lua coroutine would */
            lua_pushliteral(L, "suspended");
            return 1;
        }

        case STATE_RUNNING: {
            lua_pushliteral(L, "running");
            return 1;
        }
    }

    /* New state added, add it to the above switch */
    return luaL_error(L, "invalid state");
}

/* Cleans-up a change collected during a Lua GC cycle */
static int gc(lua_State* const L) {
    change_t* const change = check(L, 1);
    state_t const state = change->state & STATE_MASK;

    if (state == STATE_DEAD || state == STATE_PAUSED) {
        free_change(L, change);
    }
    else {
        change->state |= FLAG_COLLECTED;
    }

    return 0;
}

/* Returns an user-friendly string for the change */
static int tostring(lua_State* const L) {
    userdata_t* const ud = (userdata_t*)luaL_checkudata(L, 1, "change");

    char str[64];
    int const len = snprintf(str, sizeof(str), "change(%zu)", ud->change_index);

    lua_pushlstring(L, str, len);
    return 1;
}

/* Creates a change, to == 0 means that target values are relative */
static int create(lua_State* const L, int const to) {
    change_t* const change = alloc();

    if (change == NULL) {
        return luaL_error(L, "out of memory");
    }

    /* The first index is the object where the fields exist */
    int stack_index = 2;

    /* Iterate over pairs of <field name, target value> */
    {
        /* aux will build out field names in the change */
        char* aux = change->field_names;
        char const* const end = change->field_names + sizeof(change->field_names);

        /* Keep track of the number of fields since there's a limit */
        size_t num_fields = 0;
        size_t const max_fields = sizeof(change->delta_values) / sizeof(change->delta_values[0]);

        /* Walk the stack */
        for (;; stack_index += 2, num_fields++) {
            if (lua_type(L, stack_index) != LUA_TSTRING) {
                /* We've reached the end of the <name, value> pairs */
                break;
            }

            if (num_fields == max_fields) {
                free_change(L, change);

                return luaL_error(
                    L,
                    "too many fields, maximum is %d",
                    (int)(sizeof(change->delta_values) / sizeof(change->delta_values[0]))
                );
            }

            size_t name_length;
            char const* const field_name = lua_tolstring(L, stack_index, &name_length);

            if (name_length > end - aux - 2) {
                /* Not enough space in field_names for the new field */
                free_change(L, change);

                return luaL_error(
                    L,
                    "field names do not fit in %d characters",
                    (int)sizeof(change->field_names)
                );
            }

            /* Copy the field name */
            strcpy(aux, field_name);
            aux += name_length + 1;

            /* Get the initial field value from the object */
            lua_getfield(L, 1, field_name);

            if (!lua_isnumber(L, -1)) {
                free_change(L, change);
                return luaL_error(L, "field \"%s\" is not a number", field_name);
            }

            change->initial_values[num_fields] = lua_tonumber(L, -1);
            lua_pop(L, 1);

            /* Get the target value from the next stack index */
            if (!lua_isnumber(L, stack_index + 1)) {
                free_change(L, change);
                return luaL_error(L, "target value for field \"%s\" is not a number", field_name);
            }

            lua_Number by = lua_tonumber(L, stack_index + 1);

            if (to) {
                /**
                 * If the target value is absolute, evaluate the change amount
                 */
                by -= change->initial_values[num_fields];
            }

            change->delta_values[num_fields] = by;
        }

        *aux = 0;
    }

    /* After the <name, value> pairs comes the change duration in seconds */
    if (!lua_isnumber(L, stack_index)) {
        free_change(L, change);
        return luaL_error(L, "duration is not a number");
    }

    change->duration = lua_tonumber(L, stack_index++);

    /* Evaluate the flags */
    {
        lua_Integer flags = 0;

        if (lua_isnumber(L, stack_index)) {
            flags = lua_tointeger(L, stack_index++);
        }

        lua_Integer const ease = (flags >> 16) & 0xff;

        if (ease < 0 || ease >= sizeof(s_ease_funcs) / sizeof(s_ease_funcs[0])) {
            free_change(L, change);
            return luaL_error(L, "invalid ease function %d", (int)ease);
        }

        change->u.ease_func = s_ease_funcs[ease];
        change->state = flags;
        change->state |= flags & FLAG_PAUSED ? STATE_PAUSED : STATE_RUNNING;
    }

    /* Check if there's a callback function to call when the change ends */
    if (lua_isfunction(L, stack_index)) {
        lua_pushvalue(L, stack_index++);
        change->callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else {
        change->callback_ref = LUA_NOREF;
    }

    /* Create and initialize the userdata object */
    userdata_t* const ud = (userdata_t*)lua_newuserdata(L, sizeof(*ud));
    ud->change_index = change - s_changes;

    /* Set the userdata's metatable */
    if (luaL_newmetatable(L, "change")) {
        static const luaL_Reg methods[] = {
            {"start",  start},
            {"kill",   kill},
            {"status", status},
            {NULL,     NULL}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, gc);
        lua_setfield(L, -2, "__gc");

        lua_pushcfunction(L, tostring);
        lua_setfield(L, -2, "__tostring");
    }

    lua_setmetatable(L, -2);

    /* Make a reference to the object to change */
    lua_pushvalue(L, 1);
    change->table_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    change->elapsed_time = 0;
    return 1;
}

/* Create a change with absolute target values */
static int to(lua_State* const L) {
    return create(L, 1);
}

/* Create a change with relative target values */
static int by(lua_State* const L) {
    return create(L, 0);
}

/**
 * Update active changes, must be called regularly with the elapsed time since
 * the last call
 */
static int update(lua_State* const L) {
    lua_Number const dt = luaL_checknumber(L, 1);

    size_t i;
    size_t const count = s_num_changes;

    for (i = 0; i < count; i++) {
        change_t* const change = s_changes + i;

        if ((change->state & STATE_MASK) != STATE_RUNNING) {
            /* Only process active changes */
            continue;
        }

        change->elapsed_time += dt;
        int const finished = change->elapsed_time >= change->duration;

        if (!finished && (change->state & FLAG_AFTER)) {
            /* Changes with this flag only update when they finish */
            continue;
        }

        /* Apply the ease function */
        lua_Number const in = finished ? 1 : change->elapsed_time / change->duration;
        lua_Number const out = change->u.ease_func(in);

        /* Update the fields in the object */
        unsigned field = 0;
        char const* aux = change->field_names;
        lua_rawgeti(L, LUA_REGISTRYINDEX, change->table_ref);

        /* While there are names in field_names... */
        while (*aux != 0) {
            /* Evaluate the updated field value */
            lua_Number const current = change->initial_values[field] +
                                       change->delta_values[field] * out;

            /* Set it in the object */
            lua_pushnumber(L, current);
            lua_setfield(L, -2, aux);

            field++;
            aux += strlen(aux) + 1;
        }

        lua_pop(L, 1);

        /* Call the callback and free the change if it's finished */
        if (finished) {
            if (change->callback_ref != LUA_NOREF) {
                lua_rawgeti(L, LUA_REGISTRYINDEX, change->callback_ref);
                lua_rawgeti(L, LUA_REGISTRYINDEX, change->table_ref);
                lua_call(L, 1, 0);
            }

            if (change->state & FLAG_REPEAT) {
                change->elapsed_time -= change->duration;
            }
            else if (change->state & FLAG_COLLECTED) {
                free_change(L, change);
            }
            else {
                change->state = STATE_DEAD;
            }
        }
    }

    return 0;
}

LUALIB_API int luaopen_changeme(lua_State* const L) {
    static const luaL_Reg functions[] = {
        {"to",     to},
        {"by",     by},
        {"update", update},
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

    static struct {char const* name; lua_Integer value;} flags[] = {
        {"PAUSED", FLAG_PAUSED},
        {"REPEAT", FLAG_REPEAT},
        {"AFTER", FLAG_AFTER}
    };

    static struct {char const* name; char const* value;} info[] = {
        {"_COPYRIGHT", "Copyright (c) 2020 Andre Leiradella"},
        {"_LICENSE", "MIT"},
        {"_VERSION", "1.1"},
        {"_NAME", "changeme"},
        {"_DESCRIPTION", "A simple module to change table fields over time"}
    };

    /* Initialize the global variables */
    s_changes = NULL;
    s_num_changes = 0;
    s_reserved_changes = 0;
    s_free = NULL;

    luaL_newlib(L, functions);

    /* Register the ease constants, flags, and info in the module */
    size_t i = 0;

    for (i = 0; i < sizeof(ease_names) / sizeof(ease_names[0]); i++) {
        lua_pushinteger(L, (lua_Integer)i << 16);
        lua_setfield(L, -2, ease_names[i]);
    }

    for (i = 0; i < sizeof(flags) / sizeof(flags[0]); i++) {
        lua_pushinteger(L, flags[i].value);
        lua_setfield(L, -2, flags[i].name);
    }

    for (i = 0; i < sizeof(info) / sizeof(info[0]); i++) {
        lua_pushstring(L, info[i].value);
        lua_setfield(L, -2, info[i].name);
    }

    /* All done */
    return 1;
}
