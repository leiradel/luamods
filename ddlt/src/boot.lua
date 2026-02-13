return function(ddlt)
    local newLexer = ddlt.newLexer

    ddlt.newLexer = function(options)
        local newopts = {}

        for option, value in pairs(options) do
            newopts[option] = value
        end

        if newopts.symbols then
            local symbols = {}

            for i = 1, #newopts.symbols do
                symbols[newopts.symbols[i]] = true
            end

            newopts.symbols = symbols
        end

        local lexer, err = newLexer(newopts)

        if not lexer then
            return nil, err
        end

        if newopts.keywords then
            local language = newopts.language
            local keywords = {}

            for i = 1, #newopts.keywords do
                keywords[newopts.keywords[i]] = true
            end

            return {
                next = function(self, la)
                    local la, err = lexer:next(la)

                    if not la then
                        return nil, err
                    end

                    if la.token == '<id>' then
                        local lexeme = language == 'cpp' and la.lexeme or la.lexeme:lower()

                        if keywords[lexeme] then
                            la.token = lexeme
                        end
                    end

                    return la
                end
            }
        else
            return lexer
        end
    end

    ddlt._COPYRIGHT = 'Copyright (C) 2017-2022 Andre Leiradella'
    ddlt._LICENSE = 'MIT'
    ddlt._VERSION = '5.5.0'
    ddlt._NAME = 'ddlt'
    ddlt._URL = 'https://github.com/leiradel/luamods/ddlt'
    ddlt._DESCRIPTION = 'A generic lexer to help writing parsers using Lua'

    return ddlt
end
