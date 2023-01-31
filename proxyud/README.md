# proxyud

**proxyud** creates a full userdata object, optionally associating a value to it, and setting a metatable to it. It's purpose is to be used as a proxy to another Lua object.

The returned proxy is **not** the value passed to `new` with the given metatable, which would be pointless, it's a new userdata which has the value **associated** to it, and with the given metatable.

## Building

`make` should do the job. It will generate a shared object that can be [require](https://www.lua.org/manual/5.3/manual.html#pdf-require)d in Lua code.

## Usage

### `proxyud.new`

`proxyud.new` creates the userdata object:

```lua
proxyud.new(
    value,    -- Value that will be set as the userdata's user value, optional.

    metatable -- Metatable for the userdata object (optional, the object
              -- will not have a metatable if not informed).
)
```

Example:

```lua
local proxyud = require 'proxyud'

-- Initialized without a metatable.
local ud1 = proxyud.new(math.pi)
print(proxyud.get(ud1)) -- prints 3.1415926535898

-- Initialized with a metatable.
local ud2 = proxyud.new(math.pi, {
    __tostring = function(self)
        return tostring(proxyud.get(self))
    end,

    __call = function(self)
        print(proxyud.get(self))
    end
})

print(ud2) -- prints 3.1415926535898
ud2() -- prints 3.1415926535898

-- Initialized with a metatable.
local ud3 = proxyud.new({pi = math.pi}, {
    __index = function(self, key)
        return proxyud.get(self)[key]
    end,

    __newindex = function(self, key, value)
        error('trying to modify a const table')
    end
})

print(ud3.pi) -- prints 3.1415926535898
print(ud3.notfound) -- prints nil
ud3.pi = 3 -- raises an error
```

### `proxyud.get`

Does the same thing as `debug.getuservalue`, but checks if the argument is a full userdata. Convenience function for when the debug library is not available.

### `proxyud.identitymetatable`

Returns a metatable that forwards all metamethods to the original object. Each call to `proxyud.identitymetatable` returns a new table.

The following metamethods are not present in the identity metatable:

* `__gc`: This metamethod would call the original object's `__gc`, which would be called again when the original object is collected.
* `__mode`: This is a string rather than a function, so it's not possible to make it return the value from the original object's metatable.
* `__name`: Same as with `__mode`.

## Changelog

* 1.0.0
  * First public release
* 1.1.0
  * Added `proxyud.identitymetatable`

## License

MIT, enjoy.
