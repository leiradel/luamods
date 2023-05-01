# lrcpp

**lrcpp** is a Lua module that binds [lrcpp](https://github.com/leiradel/lrcpp) frontends to Lua programs.

## Usage

1. Include `lualrcpp.h` in your code.
1. Use `lrcpp::openf` to expose the module to Lua, it will push a table with module information and some constants.
1. Use `lrcpp::push` to push a frontend onto a Lua stack.
1. If needed, use `lrcpp::check` to get a frontend at a given Lua stack index.
1. Build `lualrcpp.cpp` along with your own code.

## Changelog

* 1.0.0
    * First public release

## License

MIT, enjoy.
