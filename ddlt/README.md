# ddlt

**ddlt** is [Lua](https://www.lua.org/) module implementing a generic lexer to help write parsers. It includes a tokenizer capable of recognizing **C++**, **BASIC**, and **Pascal** comments, identifiers, and number and string literals. A template engine is also included to ease the development of transpilers.

**ddlt** is inspired by [DDLParser](https://github.com/macton/DDLParser) and my previous work with serialization for Insomniac.

The tokenizer recognizes:

* **C++**
  * Line comments from `//` to the end of the line.
  * Block comments from `/*` to `*/`. Nested comments are *not* supported.
  * C preprocessor directives, from `#` (as long as its the first non-space character in the line) to the end of the line.
  * Identifiers in the form `[A-Za-z_][A-Za-z_0-9]*`.
  * Numbers in the form:
    * `0x[0-9A-Fa-f]+` as hexadecimal literals
    * `0b[01]+` as binary literals
    * `[1-9][0-9]*` as decimal literals
    * `0[0-7]+` as octal literals
    * `[0-9]*\.[0-9]+([Ee][+-]?[0-9]+)?` as float literals
    * `[0-9]+\.[0-9]*([Ee][+-]?[0-9]+)?` as float literals
    * `[0-9]+[Ee][+-]?[0-9]+` as float literals
    * Integer literals can be suffixed with `u`, `ul`, `ull`, `l`, `lu`, `ll`, or `llu`, in either lower and upper case
    * Float literals can be suffixed with `f` or `l`, in either lower and upper case
    * All numbers can have the decimal separator `'` at any place, except at the beginning of digit sequences or at the end of the literal
  * Strings, including the following [escape sequences](https://en.wikipedia.org/wiki/Escape_sequences_in_C):
    * `\a`, `\b`, `\f`, `\n`, `\r`, `\t`, `\v`, `\\`, `\'`, `\"`, `\?`
    * `\x` followed by at least one hexadecimal digit
    * `\u` followed by exactly four hexadecimal digits
    * `\U` followed by exactly eight hexadecimal digits
    * A `\` followed by at least one octal digit
  * Strings literals other than the narrow multibyte:
    * `L"..."` wide strings
    * `u8"..."` UTF-8 encoded strings
    * `u"..."` UTF-16 encoded strings
    * `U"..."` UTF-32 encoded strings
    * `R"..."` raw strings
    * `LR"..."` and `RL"..."` raw wide strings
    * `uR"..."` and `Ru"..."` raw UTF-16 encoded strings
    * `UR"..."` and `RU"..."` raw UTF-32 encoded strings
  * Character and multicharacter literals
    * `'...'` character
    * `L'...'` wide character
    * `u8'...'` UTF-8 encoded character
    * `u'...'` UTF-16 encoded character
    * `U'...'` UTF-32 encoded character
  * Number, string, and character literals can have user-defined suffixes in the form `_[A-Za-z_0-9]*`
* **BASIC**
  * Line comments from `'` to the end of the line.
  * Line comments from `REM`, independent of case, to the end of the line.
  * Identifiers in the form `[A-Za-z_][A-Za-z_0-9]*`.
  * Numbers in the form:
    * `&[Hh][0-9A-Fa-f]+` as hexadecimal literals
    * `[0-9]+` as decimal literals
    * `&[Oo][0-7]+` as octal literals
    * `&[Bb][01]+` as binary literals
    * `[0-9]*\.[0-9]+([Ee][+-]?[0-9]+)?` as float literals
    * `[0-9]+\.[0-9]*([Ee][+-]?[0-9]+)?` as float literals
    * `[0-9]+[Ee][+-]?[0-9]+` as float literals
    * Integer literals can be suffixed with `%`, `&`, `s`, `us`, `i`, `ui`, `l`, or `ul`, in either lower and upper case
    * Float literals can be suffixed with `@`, `!`, `#`, `f`, `r`, or `d`, in either lower and upper case
  * Strings, where `""` is interpreted as a single quote inside the string.
* **Pascal**
  * Line comments from `//` to the end of the line.
  * Block comments from `(*` to `*)`. Nested comments are *not* supported.
  * Block comments from `{` to `}`. Nested comments are *not* supported.
  * Directives from `{$` to `}`, or from `(*$` to `*)`.
  * Numbers in the form:
    * `$[0-9a-fA-F]+` as hexadecimal literals
    * `[0-9]+` as decimal literals
    * `&[0-7]+` as octal literals
    * `%[01]+` as binary literals
    * `[0-9]+\.[0-9]+` as float literals
    * `[0-9]+[Ee][+-]?[0-9]+` as float literals
    * `[0-9]+\.[0-9]+[Ee][+-]?[0-9]+` as float literals
  * Strings, where `#[0-9]+` can appear anywhere outside the single quotes to denote a character corresponding to the given number, i.e. `#65'B'` is equivalent to `'AB'`.

The tokenizer can also recognize and return *freeform* blocks, using user-defined delimiters, and which can have any content inside these delimiters.

## Build

`make` should do the job. It will generate a shared object that can be [require](https://www.lua.org/manual/5.3/manual.html#pdf-require)d in Lua code.

## Usage

If you have a Lua file with

```lua
local ddlt = require 'ddlt'

local parse = function(file)
    local inp = assert(io.open(file, 'rb'))
    local source = inp:read('*a')
    inp:close()

    local lexer = ddlt.newLexer{
        source = source,
        file = file,
        language = 'cpp',
        symbols = {'{', '}', ',', ';', '='},
        keywords = {'enum', 'struct', 'string', 'int', 'float'},
        freeform = {{'[{', '}]'}}
    }

    local tokens = {}
    local max = 0

    repeat
    local la = {}
    assert(lexer:next(la))

    la.lexeme = la.lexeme:gsub('\n', '\\n')
    tokens[#tokens + 1] = la
    max = math.max(max, #la.token)
    until la.token == '<eof>'

    tokens.max = max
    return tokens
end

local template = [[
/*! local tkfmt = '%-' .. args.max .. 's' */
/*! for i = 1, #args do */
/*!     local la = args[i] */
line = /*= string.format('%3d', la.line) */ token = /*= string.format(tkfmt, la.token) */ lexeme = /*= la.lexeme */
/*! end */
]]

if #arg ~= 1 then
    error('missing input file\n')
end

local res = {}
local tokens = parse(arg[1])
local templ = assert(ddlt.newTemplate(template, '/*', '*/'))

templ(tokens, function(line, out) res[#res + 1] = out end)
res = table.concat(res):gsub('\n+', '\n')
io.write(res)
```
  
and an input file with

```c
// The weapons available in the game
enum Weapons {
    kFist,
    kChainsaw,
    kPistol,
    kShotgun,
    kChaingun,
    kRocketLauncher,
    kPlasmaGun,
    kBFG9000
};

/* The player */
struct Hero {
    string name = "John \"Hero\" Doe";
    int health = 100;
    int armour = 0x0;
    float speed = 14.3;
    isAlive = [{
        [{
            // Nested freeform block
        }]
        return health > 0;
    }]
};
```

the result of running the parser will be

```
$ lua test.lua test.ddl

line =   1 token = <linecomment>  lexeme = // The weapons available in the game\n
line =   2 token = enum           lexeme = enum
line =   2 token = <id>           lexeme = Weapons
line =   2 token = {              lexeme = {
line =   3 token = <id>           lexeme = kFist
line =   3 token = ,              lexeme = ,
line =   4 token = <id>           lexeme = kChainsaw
line =   4 token = ,              lexeme = ,
line =   5 token = <id>           lexeme = kPistol
line =   5 token = ,              lexeme = ,
line =   6 token = <id>           lexeme = kShotgun
line =   6 token = ,              lexeme = ,
line =   7 token = <id>           lexeme = kChaingun
line =   7 token = ,              lexeme = ,
line =   8 token = <id>           lexeme = kRocketLauncher
line =   8 token = ,              lexeme = ,
line =   9 token = <id>           lexeme = kPlasmaGun
line =   9 token = ,              lexeme = ,
line =  10 token = <id>           lexeme = kBFG9000
line =  11 token = }              lexeme = }
line =  11 token = ;              lexeme = ;
line =  13 token = <blockcomment> lexeme = /* The player */
line =  14 token = struct         lexeme = struct
line =  14 token = <id>           lexeme = Hero
line =  14 token = {              lexeme = {
line =  15 token = string         lexeme = string
line =  15 token = <id>           lexeme = name
line =  15 token = =              lexeme = =
line =  15 token = <string>       lexeme = "John \"Hero\" Doe"
line =  15 token = ;              lexeme = ;
line =  16 token = int            lexeme = int
line =  16 token = <id>           lexeme = health
line =  16 token = =              lexeme = =
line =  16 token = <decimal>      lexeme = 100
line =  16 token = ;              lexeme = ;
line =  17 token = int            lexeme = int
line =  17 token = <id>           lexeme = armour
line =  17 token = =              lexeme = =
line =  17 token = <hexadecimal>  lexeme = 0x0
line =  17 token = ;              lexeme = ;
line =  18 token = float          lexeme = float
line =  18 token = <id>           lexeme = speed
line =  18 token = =              lexeme = =
line =  18 token = <float>        lexeme = 14.3
line =  18 token = ;              lexeme = ;
line =  19 token = <id>           lexeme = isAlive
line =  19 token = =              lexeme = =
line =  19 token = <freeform>     lexeme = [{\n    [{\n      // Nested freeform block\n    }]\n    return health > 0;\n  }]
line =  25 token = }              lexeme = }
line =  25 token = ;              lexeme = ;
line =  25 token = <eof>          lexeme = <eof>
```

See the `examples` and `tests` folders for unit tests, a simple tokenizer, and a simple Finite State Machine compiler written with **ddlt**.

## Documentation

Your parser can require **ddlt** to access functions to tokenize an input source code, and to create templates to generate code, as well as some functions to help deal with the file system.

* `absolute = realpath(path)`: returns the absolute path for the given path.
* `dir, name, ext = split(path)`: splits a path into its constituents, dir, file name, and extension.
* `path = join(dir, name, ext)`: joins the path constituents, forming a whole path. Any part can be nil, and will not appear in the resulting path.
* `entries = scandir(path)`: returns a table with all the entries in the specified path.
* `info = stat(path)`: returns a table with information about the object at path, as returned by [stat](https://linux.die.net/man/2/stat) containing `size`, `atime`, `mtime`, `ctime`, `sock`, `link`, `file`, `block`, `dir`, `char`, and `fifo`.
* `lexer = newLexer(options)`: returns a new tokenizer (see below).
* `template = newTemplate(code, open_tag, close_tag, name)`: returns a function that, when called, will execute the template (see below).

### newLexer

`newLexer` returns a tokenizer for a given soure code. It accepts a table with the following fields:

* `source`: a string with the entire source code that will be tokenized.
* `file`: a string with the name of the object used to create the source code (usually the file name from where the source code was read, this is used in error messages).
* `symbol`: an array of valid symbols.
* `language`: a string containing the language used to parse identifiers, string and number literals, and comments. Supported languages are `'cpp'` for **C++**, `'bas'` for **BASIC**, and `'pas'` for **Pascal**.
* `freeform`: an array where each element is a table with two elements, the *freeform* delimiters to recognize freeform blocks.

Optionally, the table can have these fields:

* `startline`: the starting line for the source code, useful for creating a lexer from a freeform block. Defaults to 1 if not given.
* `keywords`: an array of valid keywords, which will then be returned instead of the generic `<id>` token.

The resulting object only has one method, `next`. It takes a table where the information of the next produced token is stored:

* `token`: a string describing the lookahead. It can be:
  * `<id>` when the lookahead is an identifier
  * `<float>` when it's a floating point literal
  * `<binary>` when it's a binary literal
  * `<octal>` when it's an octal literal
  * `<decimal>` when it's a decimal literal
  * `<hexadecimal>` when it's a hexadecimal literal
  * `<string>` when it's a string literal
  * `<eof>` when there are no more tokens in the source code
  * `<linecomment>` and `<blockcomment>` when it's a line or a block comment
  * `<linedirective>` and `<blockdirective>` when it's a C pre-processor or Pascal directive
  * `<freeform>` when it's a *freeform* block as configured in `newLexer`
  * A symbol, as identified by the `symbol` array
  * A keyword, as identified by the `keywords` array when provided
* `lexeme`: a string with the value of the token as found in the source code
* `line`: the line number where the token is in the source code

`next` will also return the same table passed to it as an argument if successful. In case of errors, it will return `nil`, plus a string describing the error. The error message will always be in the format `<file>:<line>: message`, which is the standard way to describe errors in compilers.

Line and block comments and directives, being returned by the tokenizer, allow for iteresting things like copying preprocessor directives to the output or processing them as they appear. If they are not wanted, remove them from the token stream in your `match` parser method, i.e.:

```lua
local lexer = newLexer{
  -- ...
}

local la = {}

local parser = {
  -- ...
  
  match = function(self, token)
    if token and token ~= la.token then
      error(string.format('%u: %s expected', la.line, token))
    end

    repeat
      lexer:next(la)
    until la.token ~= '<linecomment>' and la.token ~= '<blockcomment>'

    -- ...
}
```

Nested *freeform* blocks are allowed, so it's easy to i.e. process **C++** class declarations in header files to generate code to create Lua bindings for them.

### newTemplate

Templates can be used to make it easier to generate code. The `newTemplate` method accepts three of four arguments:

* `newTemplate(code, open_tag, close_tag, name)`
  * `code`: the template source code
  * `open_tag`: the open tag that delimits special template instructions
  * `close_tag`: the close tag
  * `name`: an optional template name, which is used in error messages; `'template'` is used if this argument is not provided

There are two template tags, one to emit content to the output, and another to execute arbitrary Lua code. To emit content, use the open tag followed by `=`. To execute code, use the open tag followed by `!`.

As an example, if you use `/*` and `*/` as delimiters:

* `/*= ... */` causes `...` to be generated in the output
* `/*! ... */` causes `...` to be executed as Lua code

The return value of `newTemplate` is a Lua function that will run the template when executed. This returned function accepts two arguments, `args`, which is used to send arbitrary data to the template, and `emit`, a function which receives the template line number and the code produced by running the template (i.e. `emit(line, code)`).

Example:

```lua
local template, code = ddlt.newTemplate(source, '#', '#', path)
local source = {}
local last_line = 0

local function emit(line, code)
    if line ~= last_line then
        if #source ~= 0 and source[#source]:sub(-1, -1) ~= '\n' then
            source[#source + 1] = '\n'
        end

        source[#source + 1] = string.format('#line %d "%s"\n', line, path)
        last_line = line
    end

    source[#source + 1] = code
end

template(args, emit)
print(table.concat(source, ''))
```

## Changelog

* 5.0.0
  * Now all languages correctly detect a symbol starting with a dot in the middle of a number literal
* 4.0.1
  * Fixed the Pascal number lexer when a number is followed by the range symbol `..`
  * Fixed two apostrophes in Pascal strings denoting a single apostrophe
  * Fixed keywords in BASIC and Pascal not being case-insensitive
* 4.0.0
  * Major rewrite:
    * New code style
    * Simplyfied code
    * New error generation using `lua_error` to simplify the code path
    * Use `const` throughout the code
    * Use C99
  * Removed usage of [LuaSrcDiet](https://github.com/jirutka/luasrcdiet)
  * Fixes to the number lexers
* 3.0.0
  * Added the template line as a parameter to the `emit` function
  * `newTemplate` now returns the compiled template code as a second return value
* 2.8.2
  * Added `stateName(State)` to the FSM generated by `fsmc.lua`
  * Fixed building the transitions argument list while generating code in `fsmc.lua`
* 2.8.1
  * Added an optional starting line to use with the source code
  * Added a Finite State Machine compiler to the `examples` folder
* 2.8.0
  * *Breaking change*: the `freeform` field of the lexer options table can now specify multiple delimiters, i.e. `{{'[{', '}]'}, {'(', ')'}}`
* 2.7.0
  * Fixed not erroring on unterminated strings
  * Added decimal separator to C++
  * Better handling of suffixes
  * Added user-defined number literal suffixes to C++ numbers and strings
  * Run `boot.lua` through [LuaSrcDiet](https://github.com/jirutka/luasrcdiet) to save space
* 2.6.0
  * Added binary number literals to C++
  * Simpler code to deal with directives
  * Diferentiate line and block directives
  * Removed debug code from `lexer.c`
* 2.5.0
  * Added Pascal directives
  * Paved the way to allow for custom directives
* 2.4.1
  * Added a correct rockspec and bumped up the version
  * Fixed the older rockspecs
* 2.4.0
  * Removed the `isSymbol` function that identifies symbols, now the `newLexer` options must contain just the `symbols` array
  * Removed the `maxSymbolLength` field from the lexer options
  * Some code improvements
* 2.3.3
  * Fixed an issue where the lexer only returns a symbol if all its prefixes are also valid symbols
* 2.3.2
  * Better code to check the end of a raw C++ string
* 2.3.1
  * Fixed line comments and C preprocessor directives at the end of the file without a trailing newline
* 2.3.0
  * Added support for C preprocessor directives
* 2.2.1
  * Fixed raw strings not accepting a ')'
* 2.2.0
  * Added support for C++ character and multicharacter literals
* 2.1.0
  * Added support for string literal prefixes, wide, UTF-8, UTF-16, UTF-32, and for all raw string prefixes
* 2.0.0
  * Made **ddlt** a module
* 1.3.1
  * Allow nested *freeform* blocks
* 1.3.0
  * Comments are now returned by the tokenizer
* 1.2.0
  * Added support for **Pascal**
* 1.1.0
  * Removed square brackets from keywords
* 1.0.0
  * First proper release
