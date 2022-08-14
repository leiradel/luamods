# luaio

**luaio** allows the native implementation of streams that mimic Lua IO streams as found in the `io` module. By implementing the required functions, a stream can be used in Lua scripts and will have the exact same methods as the Lua IO streams.

## Usage

1. Implement the functions in `luaio_VirtualTable`, and define a `luaio_VirtualTable` using the created functions.
    * The functions follow the exact same semantics as the `FILE` stream functions from the C library.
    * All functions must be implemented, if you don't want/can't support the corresponding feature implement the function as just flagging an error.
1. In your own structure representing your stream, declare a field with type `luaio_Stream` as the very first field in the structure, followed by your own fields needed to implement your stream.
1. When creating the userdata object, use the following `lua_CFunction`s to create its metatable:
    * `luaio_read`
    * `luaio_write`
    * `luaio_lines`
    * `luaio_flush`
    * `luaio_seek`
    * `luaio_close`
    * `luaio_setvbuf`
    * The metamethods names are the respective `lua_CFunction`s without the `luaio_` prefix.
1. Also use `luaio_gc` as the `__gc` field in the metatable.
1. Implement `luaio_Check` that must check for a valid userdata object at the provided stack index.

> In the functions bodies, cast the `luaio_Stream` pointer to your own structure implementation.
    
The resulting userdata, when used in Lua scripts, will behave exactly as a Lua IO stream.

More than one stream can be implemented by having different `luaio_VirtualTable`s. All must share the same metatable name.

### C++

Not having a `luaio_Stream` field as the first field (offset 0 of your structure) will likely cause a crash sooner or later.

In particular, careful when using this code with C++; using a C++ class or structure with virtual methods to implement a stream can cause undefined behavior. In this case, it's better to implement everything as mentioned above, have a pointer to your stream superclass after the `luaio_Stream` field, and call its methods from the functions used in `luaio_VirtualTable`. This avoids doing pointer arithmetic to get a pointer to your C++ object.

## Building

Add `luaio.h` to your include path, and compile `luaio.c` and link the generated object file to your final executable along with your code implementing your stream.

## Todo

* Provide an example implementation.
* Implement the inverse: C functions with `FILE` semantics that use Lua streams to do the I/O.

## Changelog

* 1.0.0
    * First public release

## License

MIT, enjoy.
