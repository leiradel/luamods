# access

**access** creates constant and sealed tables with the following guarantees:

* Constant tables throw errors when a value is assigned to a field, even if the field doesn't exist
* Sealed tables throw errors when a value is assigned to a field that doesn't exist, but accepts changes to existing fields

Also, in both cases accessing an inexistent field will throw an error.

## Bulding

**access** is a pure Lua module.

## Usage

### `access.const`

Example:

```lua
local access = require 'access'

-- Create a constant table.
local c = access.const {
    pi = math.pi
}

-- Throws an error, c.huge doesn't exist.
print(c.huge)

-- Throws an error, the table is constant.
c.pi = math.huge

-- Also throws an error.
c.huge = math.huge
```

### `access.seal`

Example:

```lua
local access = require 'access'

-- Create a constant table.
local s = access.seal {
    pi = math.pi
}

-- Throws an error, c.huge doesn't exist.
print(s.huge)

-- Ok, s.pi exists and will be equal to math.huge.
s.pi = math.huge

-- Throws an error, the table is sealed.
s.huge = math.huge
```

### **access** with C code

**access** can be used to return a constant module from native C code. Constant modules cannot be changed, of course.

```c
LUAMOD_API int luaopen_module(lua_State* const L) {
    static const luaL_Reg functions[] = {
        {"init", init},
        {NULL, NULL}
    };

    // Lua code that will make a module constant if the access module exists.
    int const load_res = luaL_loadstring(L,
        "return function(module)\n"
        "    local found, access = pcall(require, 'access')\n"
        "    return found and access.const(module) or module\n"
        "end\n"
    );

    // Oops.
    if (load_res != LUA_OK) {
        return lua_error(L);
    }

    // The function in the Lua code above is on the top of the stack.
    lua_call(L, 0, 1);

    // Create and push the table with the module functions.
    luaL_newlib(L, functions);

    // Run the Lua code to make the module table constant if access exists.
    lua_call(L, 1, 1);

    // Return the module.
    return 1;
}
```

## Dependencies

**access** depends on the **proxyud** module to create full userdata objects used as proxies to the real values.

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.
