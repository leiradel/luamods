# unzip

**unzip** uncompresses entries from a ZIP archive, and calculates the CRC-32 value of Lua strings.

## Building

Only three files needs to be compiled, either add them to your project or build a loadable Lua module with:

```
$ gcc -std=c99 -O2 -Iminizip -Werror -Wall -Wpedantic -shared -fPIC -o unzip.so unzip.c minizip/unzip.c minizip/ioapi.c -lz
```

## Usage

### `unzip.init`

`unzip.init` initializes and returns a ZIP archive that can be used to read entries from it.

```lua
unzip.init(
    file -- Any value that has the read and seek functions following Lua's io
         -- library semantics.
)
```

### `:exists()`

The `exists` methods will return `true` if the informed path exists in the ZIP archive, or `false` otherwise. The check is case-sensitive.

```lua
zip:exists(
    path, -- The path to the file inside the archive.
)
```

Example:

```lua
local unzip = require 'unzip'

local file = assert(io.open('game.jar', 'rb'))
local zip = assert(unzip.init(file))

-- Only print the manifest if it exists
if zip:exists('MANIFEST') then
    zip:read('MANIFEST', io.stdout) -- or print(zip:read('MANIFEST'))
end

file:close()
```

### `:read()`

The `read` method reads entries from an open ZIP archive. Reading from an entry that represents a folder returns an empty string.

```lua
zip:read(
    path, -- The path to the file inside the archive.
    sink  -- Any value that has the write function following Lua's io library
          -- semantics. If not informed, a sink that writes to a string will be
          -- created, and the final string returned.
)
```

Example:

```lua
local unzip = require 'unzip'

local file = assert(io.open('game.jar', 'rb'))
local zip = assert(unzip.init(file))

zip:read('MANIFEST', io.stdout) -- or print(zip:read('MANIFEST'))

file:close()
```

### `:enumerate()`

The enumerate method lists all entries in the ZIP archive. For each entry, a user-provided function is called with the following information:

1. The complete file path. Folders end with a trailing slash (`'/'`), and the other arguments are all zeroed.
    * Bare in mind that empty files will also have the other arguments zeroed, so check for the trailing slash to differentiate between files and folders.
1. The compressed file size.
1. The uncompressed file size.
1. The CRC-32 of the file.

It's allowed to read from the current entry inside the callback, but reading from any other entry will confuse the iterator, making it return without enumerating all the entries or going into an infinite loop.

```lua
zip:enumerate(
    callback -- The callback that will receive each entry's information. It can
             -- any value that is callable.
)
```

Example:

```lua
local unzip = require 'unzip'

local file = assert(io.open('game.jar', 'rb'))
local zip = assert(unzip.init(file))

zip:enumerate(function(filename, compressedSize, uncompressedSize, crc32)
    local calculated = crc = unzip.crc32(zip:read(filename))
    assert(crc32 == calculated)
    print(filename, compressedSize, uncompressedSize, crc32)
end)

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

* 2.1.0
    * Added `:enumerate()` to list all the entries in a ZIP archive
    * Fixes to the documentation
* 2.0.0
    * Fixed passing a value after a nil for the sink creating the string sink in the wrong stack index
    * Breaking: `:read()` now returns either the string if the string sink is used, or nothing at all
* 1.0.0
    * First public release

## License

MIT, enjoy.
