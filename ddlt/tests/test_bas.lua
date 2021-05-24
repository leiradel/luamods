local ddlt = require 'ddlt'

local parse = function(file)
    local inp = assert(io.open(file, 'rb'))
    local source = inp:read('*a')
    inp:close()

    local lexer = ddlt.newLexer{
        source = source,
        file = file,
        language = 'bas',
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
--[[!     local la = args[i] ]]
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
    {line = 1, index = 1, token = [[<linecomment>]], lexeme = "' Line comment\
"},
    {line = 3, index = 17, token = [[<linecomment>]], lexeme = "rem Line comment\
"},
    {line = 5, index = 35, token = [[<linecomment>]], lexeme = "REM Line comment\
"},
    {line = 7, index = 53, token = [[<freeform>]], lexeme = "[{\
  Free\
  form\
  block\
\
  [{ Nested freeform block }]\
}]"},
    {line = 15, index = 113, token = [[<id>]], lexeme = "id"},
    {line = 16, index = 116, token = [[<id>]], lexeme = "Id"},
    {line = 17, index = 119, token = [[<id>]], lexeme = "iD"},
    {line = 18, index = 122, token = [[<id>]], lexeme = "ID"},
    {line = 19, index = 125, token = [[<id>]], lexeme = "_id"},
    {line = 20, index = 129, token = [[<id>]], lexeme = "id_"},
    {line = 21, index = 133, token = [[<id>]], lexeme = "_"},
    {line = 23, index = 136, token = [[<decimal>]], lexeme = "0"},
    {line = 24, index = 138, token = [[<hexadecimal>]], lexeme = "&habcd"},
    {line = 25, index = 145, token = [[<hexadecimal>]], lexeme = "&HABCD"},
    {line = 26, index = 152, token = [[<octal>]], lexeme = "&o123"},
    {line = 27, index = 158, token = [[<octal>]], lexeme = "&O123"},
    {line = 28, index = 164, token = [[<binary>]], lexeme = "&b01"},
    {line = 29, index = 169, token = [[<binary>]], lexeme = "&B01"},
    {line = 30, index = 174, token = [[<decimal>]], lexeme = "123"},
    {line = 31, index = 178, token = [[<float>]], lexeme = "1."},
    {line = 32, index = 181, token = [[<float>]], lexeme = ".1"},
    {line = 33, index = 184, token = [[<float>]], lexeme = "1.1"},
    {line = 34, index = 188, token = [[<float>]], lexeme = "1.f"},
    {line = 35, index = 192, token = [[<float>]], lexeme = ".1f"},
    {line = 36, index = 196, token = [[<float>]], lexeme = "1.1f"},
    {line = 37, index = 201, token = [[<float>]], lexeme = "1.e1"},
    {line = 38, index = 206, token = [[<float>]], lexeme = ".1e1"},
    {line = 39, index = 211, token = [[<float>]], lexeme = "1.1e1"},
    {line = 40, index = 217, token = [[<float>]], lexeme = "1.e1f"},
    {line = 41, index = 223, token = [[<float>]], lexeme = ".1e1f"},
    {line = 42, index = 229, token = [[<float>]], lexeme = "1.1e1f"},
    {line = 43, index = 236, token = [[<float>]], lexeme = "1.r"},
    {line = 44, index = 240, token = [[<float>]], lexeme = "1.d"},
    {line = 45, index = 244, token = [[<float>]], lexeme = "1.@"},
    {line = 46, index = 248, token = [[<float>]], lexeme = "1.!"},
    {line = 47, index = 252, token = [[<float>]], lexeme = "1.#"},
    {line = 48, index = 256, token = [[<decimal>]], lexeme = "1s"},
    {line = 49, index = 259, token = [[<decimal>]], lexeme = "1us"},
    {line = 50, index = 263, token = [[<decimal>]], lexeme = "1i"},
    {line = 51, index = 266, token = [[<decimal>]], lexeme = "1ui"},
    {line = 52, index = 270, token = [[<decimal>]], lexeme = "1l"},
    {line = 53, index = 273, token = [[<decimal>]], lexeme = "1ul"},
    {line = 54, index = 277, token = [[<decimal>]], lexeme = "1%"},
    {line = 55, index = 280, token = [[<decimal>]], lexeme = "1&"},
    {line = 56, index = 283, token = [[<decimal>]], lexeme = "1UL"},
    {line = 57, index = 287, token = [[<float>]], lexeme = "1#"},
    {line = 59, index = 291, token = [[<string>]], lexeme = "\"\""},
    {line = 60, index = 294, token = [[<string>]], lexeme = "\"\"\"\""},
    {line = 61, index = 299, token = [[<string>]], lexeme = "\"\"\"a\"\"\""},
    {line = 62, index = 307, token = [[<string>]], lexeme = "\"a\"\"\""},
    {line = 63, index = 313, token = [[<string>]], lexeme = "\"\"\"a\""},
    {line = 64, index = 319, token = [[<string>]], lexeme = "\"a\"\"b\""},
    {line = 64, index = nil, token = [[<eof>]], lexeme = nil}
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
