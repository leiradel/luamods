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
        la.lexeme = la.lexeme:gsub('\n', '\\n')
        tokens[#tokens + 1] = la
        max = math.max(max, #la.token)
    until la.token == '<eof>'

    tokens.max = max
    return tokens
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
local tokens = parse(arg[1])
local templ = assert(ddlt.newTemplate(template, '--[[', ']]'))

templ(tokens, function(line, out) res[#res + 1] = out end)
res = table.concat(res):gsub('\n+', '\n')
--io.write(res)

-- inception
local expected = {
    {line = 1, token = [[<linecomment>]], lexeme = [[// Line comment\n]]},
    {line = 3, token = [[<blockcomment>]], lexeme = [[(* Block comment *)]]},
    {line = 5, token = [[<blockcomment>]], lexeme = [[{ Block comment\nspawning multiple\nlines }]]},
    {line = 9, token = [[<blockdirective>]], lexeme = [[{$line test_cpp.ddl 9}]]},
    {line = 11, token = [[<blockdirective>]], lexeme = [[(*$ifndef DEBUG*)]]},
    {line = 12, token = [[<blockdirective>]], lexeme = [[(*$define assert(x) do { x; } while (0)*)]]},
    {line = 12, token = [[<blockdirective>]], lexeme = [[{$define NDEBUG}]]},
    {line = 13, token = [[<blockdirective>]], lexeme = [[(*$endif*)]]},
    {line = 15, token = [[<freeform>]], lexeme = [=[[{\n  Free\n  form\n  block\n\n  [{ Nested freeform block }]\n}]]=]},
    {line = 23, token = [[<id>]], lexeme = [[id]]},
    {line = 24, token = [[<id>]], lexeme = [[Id]]},
    {line = 25, token = [[<id>]], lexeme = [[iD]]},
    {line = 26, token = [[<id>]], lexeme = [[ID]]},
    {line = 27, token = [[<id>]], lexeme = [[_id]]},
    {line = 28, token = [[<id>]], lexeme = [[id_]]},
    {line = 29, token = [[<id>]], lexeme = [[_]]},
    {line = 31, token = [[<decimal>]], lexeme = [[0]]},
    {line = 32, token = [[<hexadecimal>]], lexeme = [[$abcd]]},
    {line = 33, token = [[<hexadecimal>]], lexeme = [[$ABCD]]},
    {line = 34, token = [[<octal>]], lexeme = [[&123]]},
    {line = 35, token = [[<binary>]], lexeme = [[%01]]},
    {line = 36, token = [[<decimal>]], lexeme = [[123]]},
    {line = 37, token = [[<float>]], lexeme = [[1.1]]},
    {line = 38, token = [[<float>]], lexeme = [[1e1]]},
    {line = 39, token = [[<float>]], lexeme = [[1.1e1]]},
    {line = 40, token = [[<string>]], lexeme = [['']]},
    {line = 41, token = [[<string>]], lexeme = [['a']]},
    {line = 42, token = [[<string>]], lexeme = [['ab']]},
    {line = 43, token = [[<string>]], lexeme = [[#48]]},
    {line = 44, token = [[<string>]], lexeme = [[#48'']]},
    {line = 45, token = [[<string>]], lexeme = [[''#48]]},
    {line = 46, token = [[<string>]], lexeme = [[''#48'']]},
    {line = 47, token = [[<string>]], lexeme = [['a'#48]]},
    {line = 48, token = [[<string>]], lexeme = [[#48'a']]},
    {line = 49, token = [[<string>]], lexeme = [['a'#48'b']]},
    {line = 50, token = [[<string>]], lexeme = [[#48#49]]},
    {line = 51, token = [[<string>]], lexeme = [[#48#49'']]},
    {line = 52, token = [[<string>]], lexeme = [[''#48#49]]},
    {line = 53, token = [[<string>]], lexeme = [[''#48#49'']]},
    {line = 54, token = [[<string>]], lexeme = [['a'#48#49]]},
    {line = 55, token = [[<string>]], lexeme = [[#48#49'a']]},
    {line = 56, token = [[<string>]], lexeme = [['a'#48#49'b']]},
    {line = 56, token = [[<eof>]], lexeme = [[<eof>]]},
}

assert(#tokens == #expected, string.format('Wrong number of tokens produced, got %d, expected %d', #tokens, #expected))

for i = 1, #tokens do
    local la1 = tokens[i]
    local la2 = expected[i]

    local msg1 = string.format('{line = %d, token = [[%s]], lexeme = [[%s]]}', la1.line, la1.token, la1.lexeme)
    local msg2 = string.format('{line = %d, token = [[%s]], lexeme = [[%s]]}', la2.line, la2.token, la2.lexeme)

    if la1.line ~= la2.line then
        io.write(string.format('Lines are different in\n  parsed:   %s\n  expected: %s\n', msg1, msg2))
        os.exit(1)
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
