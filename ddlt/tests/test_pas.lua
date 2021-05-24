local ddlt = require 'ddlt'

local parse = function(file)
    local inp = assert(io.open(file, 'rb'))
    local source = inp:read('*a')
    inp:close()

    local lexer = ddlt.newLexer{
        source = source,
        file = file,
        language = 'pas',
        symbols = {},
        freeform = {{'[{', '}]'}}
    }

    local tokens = {}
    local max = 0

    repeat
        local la = {}
        assert(lexer:next(la))
        tokens[#tokens + 1] = la
        max = math.max(max, #la.token)
    until la.token == '<eof>'

    tokens.max = max
    return tokens, source
end

local template = [===[
local expected = {
--[[! for i = 1, #args do ]]
--[[!     local la = args[ i ] ]]
    {line = --[[= la.line ]], token = [[--[[= la.token ]]]], lexeme = [[--[[= la.lexeme ]]]]},
--[[! end ]]
}
]===]

if #arg ~= 1 then
    error('missing input file\n')
end

local res = {}
local tokens, source = parse(arg[1])
local templ = assert(ddlt.newTemplate(template, '--[[', ']]'))

templ(tokens, function(line, out) res[#res + 1] = out end)
res = table.concat(res):gsub('\n+', '\n')
--io.write(res)

-- inception
local expected = {
    {line = 1, index = 1, token = [[<linecomment>]], lexeme = "// Line comment\
"},
    {line = 3, index = 18, token = [[<blockcomment>]], lexeme = "(* Block comment *)"},
    {line = 5, index = 39, token = [[<blockcomment>]], lexeme = "{ Block comment\
spawning multiple\
lines }"},
    {line = 9, index = 82, token = [[<blockdirective>]], lexeme = "{$line test_cpp.ddl 9}"},
    {line = 11, index = 106, token = [[<blockdirective>]], lexeme = "(*$ifndef DEBUG*)"},
    {line = 12, index = 126, token = [[<blockdirective>]], lexeme = "(*$define assert(x) do { x; } while (0)*)"},
    {line = 12, index = 167, token = [[<blockdirective>]], lexeme = "{$define NDEBUG}"},
    {line = 13, index = 184, token = [[<blockdirective>]], lexeme = "(*$endif*)"},
    {line = 15, index = 196, token = [[<freeform>]], lexeme = "[{\
  Free\
  form\
  block\
\
  [{ Nested freeform block }]\
}]"},
    {line = 23, index = 256, token = [[<id>]], lexeme = "id"},
    {line = 24, index = 259, token = [[<id>]], lexeme = "Id"},
    {line = 25, index = 262, token = [[<id>]], lexeme = "iD"},
    {line = 26, index = 265, token = [[<id>]], lexeme = "ID"},
    {line = 27, index = 268, token = [[<id>]], lexeme = "_id"},
    {line = 28, index = 272, token = [[<id>]], lexeme = "id_"},
    {line = 29, index = 276, token = [[<id>]], lexeme = "_"},
    {line = 31, index = 279, token = [[<decimal>]], lexeme = "0"},
    {line = 32, index = 281, token = [[<hexadecimal>]], lexeme = "$abcd"},
    {line = 33, index = 287, token = [[<hexadecimal>]], lexeme = "$ABCD"},
    {line = 34, index = 293, token = [[<octal>]], lexeme = "&123"},
    {line = 35, index = 298, token = [[<binary>]], lexeme = "%01"},
    {line = 36, index = 302, token = [[<decimal>]], lexeme = "123"},
    {line = 37, index = 306, token = [[<float>]], lexeme = "1.1"},
    {line = 38, index = 310, token = [[<float>]], lexeme = "1e1"},
    {line = 39, index = 314, token = [[<float>]], lexeme = "1.1e1"},
    {line = 40, index = 320, token = [[<string>]], lexeme = "''"},
    {line = 41, index = 323, token = [[<string>]], lexeme = "'a'"},
    {line = 42, index = 327, token = [[<string>]], lexeme = "'ab'"},
    {line = 43, index = 332, token = [[<string>]], lexeme = "#48"},
    {line = 44, index = 336, token = [[<string>]], lexeme = "#48''"},
    {line = 45, index = 342, token = [[<string>]], lexeme = "''#48"},
    {line = 46, index = 348, token = [[<string>]], lexeme = "''#48''"},
    {line = 47, index = 356, token = [[<string>]], lexeme = "'a'#48"},
    {line = 48, index = 363, token = [[<string>]], lexeme = "#48'a'"},
    {line = 49, index = 370, token = [[<string>]], lexeme = "'a'#48'b'"},
    {line = 50, index = 380, token = [[<string>]], lexeme = "#48#49"},
    {line = 51, index = 387, token = [[<string>]], lexeme = "#48#49''"},
    {line = 52, index = 396, token = [[<string>]], lexeme = "''#48#49"},
    {line = 53, index = 405, token = [[<string>]], lexeme = "''#48#49''"},
    {line = 54, index = 416, token = [[<string>]], lexeme = "'a'#48#49"},
    {line = 55, index = 426, token = [[<string>]], lexeme = "#48#49'a'"},
    {line = 56, index = 436, token = [[<string>]], lexeme = "'a'#48#49'b'"},
    {line = 56, index = nil, token = [[<eof>]], lexeme = nil}
}

assert(#tokens == #expected, string.format('Wrong number of tokens produced, got %d, expected %d', #tokens, #expected))

for i = 1, #tokens do
    local la1 = tokens[i]
    local la2 = expected[i]

    local msg1 = string.format('{line = %d, index = %d, token = [[%s]], lexeme = %q}', la1.line, la1.index or 0, la1.token, la1.lexeme)
    local msg2 = string.format('{line = %d, index = %d, token = [[%s]], lexeme = %q}', la2.line, la2.index or 0, la2.token, la2.lexeme)

    if la1.line ~= la2.line then
        io.write(string.format('Lines are different in\n  parsed:   %s\n  expected: %s\n', msg1, msg2))
        os.exit(1)
    end

    if la1.index ~= la2.index then
        io.write(string.format('Indices are different in\n  parsed:   %s\n  expected: %s\n', msg1, msg2))
        os.exit(1)
    end

    if la1.index then
        local lexeme = source:sub(la1.index, la1.index + #la1.lexeme - 1)

        if lexeme ~= la1.lexeme then
            io.write(string.format('Index is wrong in\n  computed: %s\n  expected: %s\n', lexeme, la1.lexeme))
            os.exit(1)
        end
    end

    if la1.token ~= la2.token then
        io.write(string.format('Tokens are different in\n  parsed:   %s\n  expected: %s\n', msg1, msg2))
        os.exit(1)
    end

    if la1.lexeme ~= la2.lexeme then
        io.write(string.format('Lexemes are different in\n  parsed:   %s\n  expected: %s\n', msg1, msg2))
        os.exit(1)
    end
end

io.write('-- tests passed\n')
