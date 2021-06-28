# proxyud

**proxyud** creates a full userdata object, optionally setting a metatable to it. It's purpose is to be used as a proxy to another Lua object.

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
local ud1 = proxyud.new()

-- Initialized with a metatable.
local ud2 = proxyud.new(nil, {__call = function() print 'called' end})

-- Prints 'called'
ud2()
```

### `proxyud.get`

Does the same thing as `debug.getuservalue`, but checks if the argument is a full userdata. Convenience function for when the debug library is not available.

Example:

```lua
local proxyud = require 'proxyud'
local ud = proxyud.new(math.pi)

-- Prints ~3.1415926535898
print(proxyud.get(ud))
```

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.
