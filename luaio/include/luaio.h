#ifndef LUAIO_H__
#define LUAIO_H__

#include <lua.h>
#include <lauxlib.h>
#include <stddef.h>

typedef struct luaio_Stream luaio_Stream;

typedef struct {
    /* The functions below follow the exact same semantics as the standard stream functions */
    int (*getc)(luaio_Stream* const stream);
    int (*ungetc)(int c, luaio_Stream* const stream);
    int (*fseek)(luaio_Stream* const stream, long offset, int whence);
    long (*ftell)(luaio_Stream* const stream);
    size_t (*fread)(void* const ptr, size_t const size, size_t const nmemb, luaio_Stream* const stream);
    size_t (*fwrite)(void const* const ptr, size_t const size, size_t const nmemb, luaio_Stream* const stream);
    int (*setvbuf)(luaio_Stream* const stream, char* const buf, int const mode, size_t const size);
    int (*fflush)(luaio_Stream* const stream);
    int (*ferror)(luaio_Stream* const stream);
    void (*clearerr)(luaio_Stream* const stream);
    int (*fclose)(luaio_Stream* const stream);
}
luaio_VirtualTable;

/* Make this structure the first member of your custom stream structure */
struct luaio_Stream {
    /* Make vtable point to your implementation of the stream functions */
    luaio_VirtualTable const* vtable;
    /* Initialize this to 0 when creating a stream */
    int isclosed;
};

/* Implement this somewhere to check for and return a stream */
luaio_Stream* luaio_Check(lua_State* const L, int const ndx);

/* Create your metatable with the methods below */
extern lua_CFunction const luaio_read;
extern lua_CFunction const luaio_write;
extern lua_CFunction const luaio_lines;
extern lua_CFunction const luaio_flush;
extern lua_CFunction const luaio_seek;
extern lua_CFunction const luaio_close;
extern lua_CFunction const luaio_setvbuf;

/* Use this gc metamethod */
extern lua_CFunction const luaio_gc;

#endif /* LUAIO_H__ */
