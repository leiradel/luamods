# buffer

**buffer** creates a read-only binary array from a string, and allows reads of data types from it. All functions throw errors in case something goes wrong.

## Building

It's just one file, either add it to your project or build a loadable Lua module with:

```
$ gcc -std=c99 -O2 -Werror -Wall -Wpedantic -shared -fPIC -o buffer.so buffer.c
```

## Usage

### `buffer.new(string)`

`buffer.new` creates the buffer object, with the read position set to 0:

```lua
buffer.new(
    value,    -- String whose contents will be available for reading via the buffer.
)
```

Example:

```lua
local buffer = require 'buffer'

local buf = assert(buffer.new('abcd'))
```

Returns the created buffer, or `nil` plus an error message in case of errors.

### `buffer.read(type)`

Reads a data type from the buffer, starting with the byte at the current read position. Allowed data types are:

* `sb`: Signed byte
* `ub`: Unsigned byte
* `swl`: Signed word (16 bits) in little-endian
* `swb`: Signed word in big-endian
* `uwl`: Unsigned word in little-endian
* `uwb`: Unsigned word in big-endian
* `sdl`: Signed double word (32 bits) in little-endian
* `sdb`: Signed double word in big-endian
* `udl`: Unsigned double word in little-endian
* `udb`: Unsigned double word in big-endian
* `sql`: Signed quad word (64 bits) in little-endian
* `sqb`: Signed quad word in big-endian
* `uql`: Unsigned quad word in little-endian
* `uqb`: Unsigned quad word in big-endian
* `fl`: [Float](https://en.wikipedia.org/wiki/Single-precision_floating-point_format) in little-endian
* `fb`: Float in big-endian
* `dl`: [Double](https://en.wikipedia.org/wiki/Double-precision_floating-point_format) in little-endian
* `db`: Double in big-endian
* `sw`: Signed word in the current machine's endian mode
* `uw`: Unsigned word in the current machine's endian mode
* `sd`: Signed double word in the current machine's endian mode
* `ud`: Unsigned double word in the current machine's endian mode
* `sq`: Signed quad word in the current machine's endian mode
* `uq`: Unsigned quad word in the current machine's endian mode
* `f`: Float in the current machine's endian mode
* `d`: Double in the current machine's endian mode
* `l`: Reads an entire line terminated with the ASCII character `\n'`
* A number: Returns a string with that many characters

The read position is incremented according to the type read.

Example:

```lua
local buffer = require 'buffer'

local buf = assert(buffer.new('\x12\x34\x56\x78'))
print(string.format('%x', assert(buf:read('udb')))) -- prints 12345678
```

Returns the read result, or `nil` plus an error message in case of errors.

### `buffer.seek(offset, whence)`

Changes the current read position. The way `offset` is applied depends on the `whence` parameter:

* `"set"` (default if omitted): The new read position will be the value of `offset`
* `"cur"`: The new read position will be `offset` added to the current read position
* `"end"`: The new read position will be the size of the buffer subtracted from `offset`

Returns the new absolute read position, or `nil` plus an error message in case of errors.

### `buffer.tell`

Returns the current read position.

### `buffer.size`

Returns the buffer size.

### `buffer.sub(begin, size)`

Returns a piece of a buffer as a new buffer, starting at the original buffer `begin` position, with `size` bytes, or `nil` plus an error message in case of errors.

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.
