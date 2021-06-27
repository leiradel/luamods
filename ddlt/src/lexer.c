#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>

#define MAX_BLOCKS 16
#define DELIM_SIZE 16

typedef struct Lexer Lexer;

typedef int (*Next)(lua_State*, Lexer*);

typedef enum {
    LINE_COMMENT,
    BLOCK_COMMENT,
    FREE_FORM,
    LINE_DIRECTIVE,
    BLOCK_DIRECTIVE
}
BlockType;

typedef struct {
    char filler;
}
LineComment;

typedef struct {
    char end[DELIM_SIZE];
}
BlockComment;

typedef struct {
    char end[DELIM_SIZE];
}
FreeForm;

typedef struct {
    int at_start;
}
LineDirective;

typedef struct {
    char end[DELIM_SIZE];
}
BlockDirective;

typedef struct {
    BlockType type;
    char begin[DELIM_SIZE];

    union {
        LineComment line_comment;
        BlockComment block_comment;
        FreeForm free_form;
        LineDirective line_directive;
        BlockDirective block_directive;
    };
}
Block;

struct Lexer {
    const char* source_name;
    unsigned line;
    const char* source_start;
    const char* source;
    const char* line_start;
    unsigned la_line;
    ptrdiff_t la_offset;

    int source_ref;
    int source_name_ref;
    int symbols_ref;

    char* symbol_chars;

    Next next;
    Block blocks[MAX_BLOCKS];
    unsigned num_blocks;
};

#define SPACE " \f\r\t\v" /* \n is treated separately to keep track of the line number */
#define ALPHA "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_"
#define DIGIT "0123456789"
#define ALNUM ALPHA DIGIT
#define XDIGIT DIGIT "ABCDEFabcdef"
#define ODIGIT "01234567"
#define BDIGIT "01"
#define NOTDELIM " ()\\\t\v\f\n"

static int error(lua_State* const L, Lexer const* const self, char const* const format, ...) {
    lua_pushfstring(L, "%s:%d: ", self->source_name, self->line);

    va_list args;
    va_start(args, format);

    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    lua_pushstring(L, buffer);
    lua_concat(L, 2);
    return lua_error(L);
}

static int pushl(lua_State* const L,
                 Lexer const* const self,
                 char const* const token,
                 size_t const token_length,
                 char const* const lexeme,
                 size_t const lexeme_length) {

    lua_pushlstring(L, token, token_length);
    lua_setfield(L, 2, "token");

    lua_pushlstring(L, lexeme, lexeme_length);
    lua_setfield(L, 2, "lexeme");

    lua_pushinteger(L, self->la_line);
    lua_setfield(L, 2, "line");

    lua_pushinteger(L, self->la_offset);
    lua_setfield(L, 2, "offset");

    lua_pushvalue(L, 2);
    return 1;
}

static int push(lua_State* const L,
                Lexer const* const self,
                char const* const token,
                char const* const lexeme,
                size_t const lexeme_length) {

    return pushl(L, self, token, strlen(token), lexeme, lexeme_length);
}

static int line_comment(lua_State* const L, Lexer* const self, Block const* const block, int const is_directive) {
    if (is_directive) {
        if (block->line_directive.at_start && self->source != self->line_start) {
            return error(L, self, "directives must start at the beginning of the line");
        }

        if (strspn(self->line_start, SPACE) != (self->source - self->line_start)) {
            return error(L, self, "directives must be the only thing in a line");
        }
    }

    char const* const lexeme = is_directive ? self->line_start : self->source;
    char const* const newline = strchr(lexeme, '\n');

    if (newline != NULL) {
        self->source = newline + 1;
        self->line_start = self->source;
        self->line++;
    }
    else {
        self->source += strlen(self->source);
    }

    if (is_directive) {
        return push(L, self, "<linedirective>", lexeme, self->source - lexeme);
    }
    else {
        return push(L, self, "<linecomment>", lexeme, self->source - lexeme);
    }
}

static int block_comment(lua_State* const L, Lexer* const self, Block const* const block, int const is_directive) {
    char const* const lexeme = self->source;
    char const* const end = is_directive ? block->block_directive.end : block->block_comment.end;

    char reject[3];
    reject[0] = '\n';
    reject[1] = *end;
    reject[2] = 0;

    size_t const end_len = strlen(end) - 1;

    for (;;) {
        self->source += strcspn(self->source, reject);

        if (*self->source == '\n') {
            self->source++;
            self->line++;
        }
        else if (*self->source == *end) {
            self->source++;

            if (!strncmp(self->source, end + 1, end_len)) {
                self->source += end_len;

                if (is_directive) {
                    return push(L, self, "<blockdirective>", lexeme, self->source - lexeme);
                }
                else {
                    return push(L, self, "<blockcomment>", lexeme, self->source - lexeme);
                }
            }
        }
        else {
            return error(L, self, "unterminated comment");
        }
    }
}

static int free_form(lua_State* const L, Lexer* const self, Block const* const block) {
    unsigned nested = 0;

    char const* const lexeme = self->source;
    char const* const begin = block->begin;
    char const* const end = block->free_form.end;

    char reject[4];
    reject[0] = '\n';
    reject[1] = *begin;
    reject[2] = *end;
    reject[3] = 0;

    size_t const begin_len = strlen(begin) - 1;
    size_t const end_len = strlen(end) - 1;

    self->source += begin_len + 1;

    for (;;) {
        self->source += strcspn(self->source, reject);

        if (*self->source == '\n') {
            self->source++;
            self->line++;
        }
        else if (*self->source == *begin) {
            self->source++;

            if (!strncmp(self->source, begin + 1, begin_len)) {
                self->source += begin_len;
                nested++;
            }
        }
        else if (*self->source == *end) {
            self->source++;

            if (!strncmp(self->source, end + 1, end_len)) {
                self->source += end_len;

                if (nested == 0) {
                    return push(L, self, "<freeform>", lexeme, self->source - lexeme);
                }

                nested--;
            }
        }
        else {
            return error(L, self, "unterminated free-form block");
        }
    }
}

static int get_suffix(lua_State* const L, Lexer* const self, char const* const suffixes) {
    for (size_t i = 0; suffixes[i] != 0; i++) {
        int match = 1;

        for (size_t j = 0;; i++, j++) {
            char const k = suffixes[i];

            if (k == ',') {
                if (match && strspn(self->source + j, ALPHA) == 0) {
                    self->source += j;
                    return 0;
                }
                else {
                    break;
                }
            }

            match = match && toupper(self->source[j]) == toupper(k);
        }
    }

    return -1;
}

static size_t get_symbol_length(lua_State* const L, Lexer* const self) {
    char const* const begin = self->source;
    size_t const length = strspn(self->source, self->symbol_chars);
    lua_rawgeti(L, LUA_REGISTRYINDEX, self->symbols_ref);

    for (size_t i = length; i > 0; i--) {
        lua_pushlstring(L, begin, i);
        lua_rawget(L, -2);
        int const found = lua_toboolean(L, -1);
        lua_pop(L, 1);

        if (found) {
            lua_pop(L, 1);
            return i;
        }
    }

    lua_pop(L, 1);
    return 0;
}

static int is_decimal_dot(lua_State* const L, Lexer* const self) {
    return *self->source == '.' && get_symbol_length(L, self) <= 1;
}

static int l_next_lang(lua_State* const L) {
    Lexer* const self = luaL_checkudata(L, 1, "lexer");
    luaL_checktype(L, 2, LUA_TTABLE);
    return self->next(L, self);
}

static int l_next(lua_State* const L) {
    Lexer* const self = luaL_checkudata(L, 1, "lexer");
    luaL_checktype(L, 2, LUA_TTABLE);

    // skip spaces and line separators
    for (;;) {
        self->source += strspn(self->source, SPACE);

        if (*self->source == 0) {
            // found the end of the input
            return push(L, self, "<eof>", "<eof>", strlen("<eof>"));
        }
        else if (*self->source == '\n') {
            self->source++;
            self->line++;
            self->line_start = self->source;
        }
        else {
            break;
        }
    }

    self->la_line = self->line;
    self->la_offset = self->source - self->source_start;

    // look for comments, free-form blocks, and directives
    for (unsigned i = 0; i < self->num_blocks; i++) {
        Block const* const block = self->blocks + i;
        char const* const begin = block->begin;

        if (!strncmp(self->source, begin, strlen(begin))) {
            switch (block->type) {
                case LINE_COMMENT: return line_comment(L, self, block, 0);
                case BLOCK_COMMENT: return block_comment(L, self, block, 0);
                case FREE_FORM: return free_form(L, self, block);
                case LINE_DIRECTIVE: return line_comment(L, self, block, 1);
                case BLOCK_DIRECTIVE: return block_comment(L, self, block, 1);
            }
        }
    }

    // try to find a symbol
    size_t const symbol_length = get_symbol_length(L, self);

    if (symbol_length != 0) {
        char const* const symbol = self->source;
        self->source += symbol_length;
        return pushl(L, self, symbol, symbol_length, symbol, symbol_length);
    }

    // call the language specific lexer
    lua_pushcfunction(L, l_next_lang);
    lua_pushvalue(L, 1);
    lua_pushvalue(L, 2);

    if (lua_pcall(L, 2, 1, 0) == LUA_OK) {
        return 1;
    }

    // error
    lua_pushnil(L);
    lua_insert(L, -2);
    return 2;
}

static int l_gc(lua_State* const L) {
    Lexer const* const self = (Lexer*)lua_touserdata(L, 1);

    luaL_unref(L, LUA_REGISTRYINDEX, self->source_ref);
    luaL_unref(L, LUA_REGISTRYINDEX, self->source_name_ref);
    luaL_unref(L, LUA_REGISTRYINDEX, self->symbols_ref);

    free(self->symbol_chars);
    return 0;
}

#include "lexer_cpp.c"
#include "lexer_bas.c"
#include "lexer_pas.c"

static int init_source(lua_State* const L, Lexer* const self) {
    lua_getfield(L, 1, "source");

    if (!lua_isstring(L, -1)) {
        return luaL_error(L, "source must be a string");
    }

    self->source = lua_tostring(L, -1);
    self->source_start = self->source;
    self->source_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    self->line_start = self->source;

    lua_getfield(L, 1, "startline");
    self->line = luaL_optinteger(L, -1, 1);
    lua_pop(L, 1);
  
    return 0;
}

static int init_file(lua_State* const L, Lexer* const self) {
    lua_getfield(L, 1, "file");

    if (!lua_isstring(L, -1)) {
        return luaL_error(L, "file name must be a string");
    }

    self->source_name = lua_tostring(L, -1);
    self->source_name_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}

static int init_symbol_chars(lua_State* const L, Lexer* const self) {
    lua_getfield(L, 1, "symbols");

    if (!lua_istable(L, -1)) {
        return luaL_error(L, "symbols must be a table");
    }

    char used[256 - 32 + 1];
    memset(used, 0, sizeof(used));

    lua_pushvalue(L, -1);
    self->symbols_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pushnil(L);

    while (lua_next(L, -2) != 0) {
        if (lua_isstring(L, -2)) {
            char const* const symbol = lua_tostring(L, -2);

            for (char const* aux = symbol; *aux != 0; aux++) {
                unsigned char const k = (unsigned char)*aux;

                if (k >= 32 && k < 256) {
                    used[k - 32] = 1;
                }
            }
        }

        lua_pop(L, 1);
    }

    lua_pop(L, 1);

    for (unsigned i = 0, j = 0; i < 256 - 32; i++) {
        if (used[i] != 0) {
            used[j++] = (char)(i + 32);
            used[j] = 0;
        }
    }

    self->symbol_chars = strdup(used);

    if (self->symbol_chars == NULL) {
        return luaL_error(L, "out of memory");
    }

    return 0;
}

static int init_language(lua_State* const L, Lexer* const self) {
    lua_getfield(L, 1, "language");

    if (!lua_isstring(L, -1)) {
        return luaL_error(L, "language name must be a string");
    }

    char const* const language = lua_tostring(L, -1);

    if (!strcmp(language, "cpp")) {
        cpp_setup_lexer(self);
    }
    else if (!strcmp(language, "bas")) {
        bas_setup_lexer(self);
    }
    else if (!strcmp(language, "pas")) {
        pas_setup_lexer(self);
    }
    else {
        return luaL_error(L, "invalid language %s", language);
    }

    lua_pop(L, 1);
    return 0;
}

static int init_freeform(lua_State* const L, Lexer* const self) {
    lua_getfield(L, 1, "freeform");

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }

    for (int i = 1;; i++) {
        lua_rawgeti(L, -1, i);

        if (lua_isnil(L, -1)) {
            lua_pop(L, 2);

            if (i == 1) {
                return luaL_error(L, "freeform array is empty");
            }

            return 0;
        }

        if (!lua_istable(L, -1))
        {
            lua_pop(L, 2);
            return luaL_error(L, "freeform array element %d is not a table", i);
        }

        lua_rawgeti(L, -1, 1);

        size_t length;
        char const* const begin = lua_tolstring(L, -1, &length);

        if (begin == NULL) {
            return luaL_error(L, "freeform begin symbol must be a string");
        }
        else if (length >= DELIM_SIZE) {
            return luaL_error(L, "freeform begin symbol is too big, maximum length is %d", DELIM_SIZE - 1);
        }

        lua_rawgeti(L, -2, 2);
        char const* const end = lua_tolstring(L, -1, &length);

        if (end == NULL) {
            return luaL_error(L, "freeform end symbol must be a string");
        }
        else if (length >= DELIM_SIZE) {
            return luaL_error(L, "freeform end symbol is too big, maximum length is %d", DELIM_SIZE - 1);
        }

        if (self->num_blocks == MAX_BLOCKS) {
            return luaL_error(L, "freeform area exhausted");
        }

        self->blocks[self->num_blocks].type = FREE_FORM;
        strcpy(self->blocks[self->num_blocks].begin, begin);
        strcpy(self->blocks[self->num_blocks].free_form.end, end);
        self->num_blocks++;

        lua_pop(L, 3);
    }
}

int l_newLexer(lua_State* const L) {
    if (!lua_istable(L, 1)) {
        return luaL_error(L, "lexer options must be a table");
    }

    lua_settop(L, 1);

    Lexer* const self = (Lexer*)lua_newuserdata(L, sizeof(Lexer));

    self->source_name_ref = LUA_NOREF;
    self->source_ref = LUA_NOREF;
    self->symbols_ref = LUA_NOREF;

    init_source(L, self);
    init_file(L, self);
    init_symbol_chars(L, self);
    init_language(L, self);
    init_freeform(L, self);

    if (luaL_newmetatable(L, "lexer") != 0) {
        static const luaL_Reg methods[] = {
            {"next", l_next},
            {NULL, NULL}
        };

        luaL_newlib(L, methods);
        lua_setfield(L, -2, "__index");

        lua_pushcfunction(L, l_gc);
        lua_setfield(L, -2, "__gc");
    }

    lua_setmetatable(L, -2);
    return 1;
}
