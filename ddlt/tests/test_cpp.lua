local ddlt = require 'ddlt'

local parse = function(file)
    local inp = assert(io.open(file, 'rb'))
    local source = inp:read('*a')
    inp:close()

    local lexer = ddlt.newLexer{
        source = source,
        file = file,
        language = 'cpp',
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
    {line = 3, index = 18, token = [[<blockcomment>]], lexeme = "/* Block comment */"},
    {line = 5, index = 39, token = [[<blockcomment>]], lexeme = "/* Block comment\
spawning multiple\
lines */"},
    {line = 9, index = 84, token = [[<linedirective>]], lexeme = "#line test_cpp.ddl 9\
"},
    {line = 11, index = 106, token = [[<linedirective>]], lexeme = "#ifndef DEBUG\
"},
    {line = 12, index = 120, token = [[<linedirective>]], lexeme = "  #define assert(x) do { x; } while (0)\
"},
    {line = 13, index = 160, token = [[<linedirective>]], lexeme = "#endif\
"},
    {line = 15, index = 168, token = [[<freeform>]], lexeme = "[{\
  Free\
  form\
  block\
\
  [{ Nested freeform block }]\
}]"},
    {line = 23, index = 228, token = [[<id>]], lexeme = "id"},
    {line = 24, index = 231, token = [[<id>]], lexeme = "Id"},
    {line = 25, index = 234, token = [[<id>]], lexeme = "iD"},
    {line = 26, index = 237, token = [[<id>]], lexeme = "ID"},
    {line = 27, index = 240, token = [[<id>]], lexeme = "_id"},
    {line = 28, index = 244, token = [[<id>]], lexeme = "id_"},
    {line = 29, index = 248, token = [[<id>]], lexeme = "_"},
    {line = 31, index = 251, token = [[<decimal>]], lexeme = "0"},
    {line = 32, index = 253, token = [[<binary>]], lexeme = "0b01010101"},
    {line = 33, index = 264, token = [[<binary>]], lexeme = "0B10101010"},
    {line = 34, index = 275, token = [[<hexadecimal>]], lexeme = "0xabcd"},
    {line = 35, index = 282, token = [[<hexadecimal>]], lexeme = "0xABCD"},
    {line = 36, index = 289, token = [[<octal>]], lexeme = "0123"},
    {line = 37, index = 294, token = [[<decimal>]], lexeme = "123"},
    {line = 38, index = 298, token = [[<float>]], lexeme = "1."},
    {line = 39, index = 301, token = [[<float>]], lexeme = ".1"},
    {line = 40, index = 304, token = [[<float>]], lexeme = "1.1"},
    {line = 41, index = 308, token = [[<float>]], lexeme = "1.f"},
    {line = 42, index = 312, token = [[<float>]], lexeme = ".1f"},
    {line = 43, index = 316, token = [[<float>]], lexeme = "1.1f"},
    {line = 44, index = 321, token = [[<float>]], lexeme = "1.e1"},
    {line = 45, index = 326, token = [[<float>]], lexeme = ".1e1"},
    {line = 46, index = 331, token = [[<float>]], lexeme = "1.1e1"},
    {line = 47, index = 337, token = [[<float>]], lexeme = "1.e1f"},
    {line = 48, index = 343, token = [[<float>]], lexeme = ".1e1f"},
    {line = 49, index = 349, token = [[<float>]], lexeme = "1.1e1f"},
    {line = 50, index = 356, token = [[<float>]], lexeme = "1.l"},
    {line = 51, index = 360, token = [[<decimal>]], lexeme = "1u"},
    {line = 52, index = 363, token = [[<decimal>]], lexeme = "1ul"},
    {line = 53, index = 367, token = [[<decimal>]], lexeme = "1ull"},
    {line = 54, index = 372, token = [[<decimal>]], lexeme = "1l"},
    {line = 55, index = 375, token = [[<decimal>]], lexeme = "1lu"},
    {line = 56, index = 379, token = [[<decimal>]], lexeme = "1llu"},
    {line = 57, index = 384, token = [[<decimal>]], lexeme = "1ULL"},
    {line = 58, index = 389, token = [[<decimal>]], lexeme = "1_km"},
    {line = 59, index = 394, token = [[<decimal>]], lexeme = "1_"},
    {line = 61, index = 398, token = [[<string>]], lexeme = "\"\""},
    {line = 62, index = 401, token = [[<string>]], lexeme = "\"\\\"\""},
    {line = 63, index = 406, token = [[<string>]], lexeme = "\"\\a\\b\\f\\n\\r\\t\\v\\\\\\'\\\"\\?\\xab\\xAB\\uabcd\\uABCD\\U0123abcd\\U0123ABCD\\033\""},
    {line = 64, index = 475, token = [[<widestring>]], lexeme = "L\"wchar_t string\""},
    {line = 65, index = 493, token = [[<utf8string>]], lexeme = "u8\"UTF-8 encoded string\""},
    {line = 66, index = 518, token = [[<utf16string>]], lexeme = "u\"char16_t string\""},
    {line = 67, index = 537, token = [[<utf32string>]], lexeme = "U\"char32_t string\""},
    {line = 68, index = 556, token = [[<rawstring>]], lexeme = "R\"raw(raw (!) string)raw\""},
    {line = 69, index = 582, token = [[<rawwidestring>]], lexeme = "LR\"raw(raw (!) wchar_t string)raw\""},
    {line = 70, index = 617, token = [[<rawwidestring>]], lexeme = "RL\"raw(raw (!) wchar_t string)raw\""},
    {line = 71, index = 652, token = [[<rawutf16string>]], lexeme = "uR\"raw(raw (!) char16_t string)raw\""},
    {line = 72, index = 688, token = [[<rawutf16string>]], lexeme = "Ru\"raw(raw (!) char16_t string)raw\""},
    {line = 73, index = 724, token = [[<rawutf32string>]], lexeme = "UR\"raw(raw (!) char32_t string)raw\""},
    {line = 74, index = 760, token = [[<rawutf32string>]], lexeme = "RU\"raw(raw (!) char32_t string)raw\""},
    {line = 75, index = 796, token = [[<char>]], lexeme = "'char character'"},
    {line = 76, index = 813, token = [[<widechar>]], lexeme = "L'wchar_t character'"},
    {line = 77, index = 834, token = [[<utf8char>]], lexeme = "u8'UTF-8 encoded character'"},
    {line = 78, index = 862, token = [[<utf16char>]], lexeme = "u'char16_t character'"},
    {line = 79, index = 884, token = [[<utf32char>]], lexeme = "U'char32_t character'"},
    {line = 80, index = 906, token = [[<string>]], lexeme = "\"press\"_zx81"},
    {line = 81, index = 919, token = [[<string>]], lexeme = "\"press\"_"},
    {line = 82, index = 928, token = [[<rawstring>]], lexeme = "R\"raw(press)raw\"_ebcdic"},
    {line = 82, index = nil, token = [[<eof>]], lexeme = "<eof>"}
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
