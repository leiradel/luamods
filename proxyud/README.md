# proxyud

**proxyud** creates a full userdata object, optionally associating a value to it, and setting a metatable to it. It's purpose is to be used as a proxy to another Lua object.

The returned proxy is **not** the value passed to `new` with the given metatable, which would be pointless, it's a new userdata which has the value **associated** to it, and with the given metatable.

## Building

It's just one file, either add it to your project or build a loadable Lua module with:

```
$ gcc -std=c99 -O2 -Werror -Wall -Wpedantic -shared -fPIC -o proxyud.so proxyud.c
```

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

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.
