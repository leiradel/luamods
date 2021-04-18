# inifile

**inifile** reads entries from a [INI](https://en.wikipedia.org/wiki/INI_file) file.

## Building

**inifile** is a pure Lua module.

## Usage

### `inifile.iterate`

Iterates over the entries of the INI file. Any object that implements a read function with the same semantics as the Lua io library can be used.

Example:

```lua
local inifile = require 'inifile'

-- Prints all <key, value> pairs in config.ini along with their section names and line numbers.
-- Errors are thrown with the error function.
local ini = assert(io.open('config.ini'))

for section, key, value, line in inifile.iterate(ini) do
    print(section, key, value, line)
end
```

### `inifile.load`

Loads an INI file and returns all entries in a table. The table has section names as the keys with their values being the <key, value> pairs for the section.

Example:

```lua
local inifile = require 'inifile'

-- Loads all entries of config.ini and return them in a table
local ini = assert(inifile.load('config.ini'))

for section, data in pairs(ini) do
    for key, value in pairs(data) do
        print(section, key, value)
    end
end
```

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.
