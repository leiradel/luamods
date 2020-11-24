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

## Examples

### Make all required modules constant

When run, the code below will install a different `require` function that will make all loaded modules constant if the **access** module is available.

```lua
local require0 = require
local found, access = pcall(require, 'access')

if found then
    require = function(modname)
        local mod = require0(modname)
        local const = access.const(mod)
        package.loaded[modname] = const
        return const
    end
end
```

### Make objects constant or sealed (C)

The code below will make the object on the top of the stack constant if the **access** module is available, otherwise it will leave the object untouched. It returns `LUA_OK` if successful, or an error code from `luaL_loadstring` if it fails (which it shouldn't), in which case an error message is left at the top of the stack.

```c
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
```

## Dependencies

**access** depends on the **proxyud** module to create full userdata objects used as proxies to the real values.

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.
