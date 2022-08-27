# crc64

**crc64** computes the CRC-64 of strings using the [ECMA-182 standard](https://www.ecma-international.org/publications-and-standards/standards/ecma-182/). CRCs are full userdata values, to avoid issues with interpreters configured to use 32-bit integers.

In addition to computing the CRC, it's possible to:

* Create a CRC from two 32-bit integers
* Directly compare two CRCs using the `==` operator
* Get an hexadecimal representation of a CRC

## Building

It's just one file, either add it to your project or build a loadable Lua module with:

```
$ gcc -std=c99 -O2 -Werror -Wall -Wpedantic -shared -fPIC -o crc64.so crc64.c
```

## Usage

```lua
local crc64 = require 'crc64'

local test1 = crc64.compute('123456789')
assert(test1 == crc64.create(0x995dc9bb, 0xdf1939fa))
print(test1) -- prints 0x995dc9bbdf1939fa

local test2 = crc64.compute('This is a test of the emergency broadcast system.')
assert(test2 == crc64.create(0x27db187f, 0xc15bbc72))
print(test2) -- prints 0x27db187fc15bbc72
```

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.

Contains source code Copyright (C) 2014, UChicago Argonne, LLC. Check `crc64.c`.
