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
/*!     local la = args[ i ] */
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
