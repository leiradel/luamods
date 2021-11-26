# unzip

**unzip** uncompresses entries from a ZIP archive, and calculates the CRC-32 value of Lua strings.

## Building

Only three files needs to be compiled, either add them to your project or build a loadable Lua module with:

```
$ gcc -std=c99 -O2 -Iminizip -Werror -Wall -Wpedantic -shared -fPIC -o unzip.so unzip.c minizip/unzip.c minizip/ioapi.c -lz
```

## Usage

### `unzip.init`

`unzip.init` init and returns a ZIP archive that can be used to read entries from it.

```lua
unzip.init(
    file -- Any value that has the read and seek functions following Lua's io
         -- library semantics.
)
```

### `:read()`

The `read` method reads entries from an open ZIP archive.

```lua
z:read(
    sink -- Any value that has the write function following Lua's io library
         -- semantics. If not informed, a sink that writes to a string will be
         -- created, and the final string returned.
)
```

Example:

```lua
local unzip = require 'unzip'

local file = assert(io.open('game.jar'))
local zip = assert(unzip.init(file))
zip:read('MANIFEST', io.stdout) -- or print(zip:read('MANIFEST'))
file:close()
```

### `unzip.crc32`

Returns the CRC-32 of the given string.

Example:

```lua
local unzip = require 'unzip'

-- Prints the CRC-32 of "test", d87f7e0c.
print(string.format('%08x', unzip.crc32('test')))
```

## Changelog

* 2.0.0
  * Fixed passing a value after a nil for the sink creating the string sink in the wrong stack index
  * Breaking: `:read()` now returns either the string if the string sink is used, or nothing at all
* 1.0.0
  * First public release

## License

MIT, enjoy.
