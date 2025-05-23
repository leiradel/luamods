local ddlt = require 'ddlt'

local function dump(t, i)
    i = i or ''

    if type(t) == 'table' then
        io.write(string.format('%s%s {\n', i, tostring(table)))

        for k, v in pairs(t) do
            io.write(string.format('%s  %s = ', i, tostring(k)))

            if type(v) == 'table' then
                io.write('\n')
                dump(v, i .. '  ')
            else
                io.write(string.format('%s', tostring(v)))
            end

            io.write('\n')
        end

        io.write(string.format('%s}\n', i))
    else
        io.write(string.format('%s%s', i, tostring(t)))
    end
end

local function fatal(path, line, format, ...)
    local location = string.format('%s:%u: ', path, line)
    local message = string.format(format, ...)
    error(string.format('%s%s', location, message))
    io.stderr:write(location, message, '\n')
    os.exit(1)
end

-------------------------------------------------------------------------------
-- Tokenizes source code
-------------------------------------------------------------------------------
local function getTokens(path, source, startLine)
    local lexer = ddlt.newLexer{
        source = source,
        startline = startLine,
        file = path,
        language = 'cpp',
        symbols = {'=>', '(', ')', ';', ','},
        keywords = {'header', 'implementation', 'fsm', 'before', 'after'},
        freeform = {{'{', '}'}}
    }

    local tokens = {}
    local la

    repeat
        repeat
            local err
            la, err = lexer:next({})

            if not la then
                io.stderr:write(err)
                os.exit(1)
            end
        until la.token ~= '<linecomment>' and la.token ~= '<blockcomment>'

        tokens[#tokens + 1] = la
    until la.token == '<eof>'

    return tokens
end

-------------------------------------------------------------------------------
-- Tokenizes a file
-------------------------------------------------------------------------------
local function tokenize(path)
    local inp, err = io.open(path, 'rb')

    if not inp then
        fatal(path, 0, 'Error opening input file: %s', err)
    end

    local source = inp:read('*a')
    inp:close()

    -- Get the top-level tokens
    return getTokens(path, source, 1)
end

-------------------------------------------------------------------------------
-- Creates a new parser with predefined error, match and line methods
-------------------------------------------------------------------------------
local function newParser(path)
    local parser = {
        init = function(self)
            self.tokens = tokenize(path)
            self.current = 1
        end,

        error = function(self, line, format, ...)
            fatal(path, line, format, ...)
        end,

        token = function(self, index)
            index = index or 1
            return self.tokens[self.current + index - 1].token
        end,

        lexeme = function(self, index)
            index = index or 1
            return self.tokens[self.current + index - 1].lexeme
        end,

        line = function(self, index)
            index = index or 1
            return self.tokens[self.current + index - 1].line
        end,

        match = function(self, token)
            if token and token ~= self:token() then
                self:error(self:line(), '"%s" expected, found "%s"', token, self:token())
            end

            self.current = self.current + 1
        end,

        expand = function(self)
            local token, lexeme, line = self:token(), self:lexeme(), self:line()

            if token ~= '<freeform>' then
                self:error(self:line(), '"{" expected, found "%s"', token)
            end

            local freeform = getTokens(path, lexeme:sub(2, -2), line)
            table.remove(self.tokens, self.current)
            table.insert(self.tokens, self.current, {line = line, token = '{', lexeme = '{'})

            for i = 1, #freeform - 1 do
                table.insert(self.tokens, self.current + i, freeform[i])
            end

            table.insert(self.tokens, self.current + #freeform, {line = line, token = '}', lexeme = '}'})
        end,

        parse = function(self)
            local fsm = {states = {}}

            -- Parse directives and statements that will be copied verbatim to
            -- the generated header and c files
            if self:token() == 'header' then
                self:match()
                fsm.header = {line = self:line(), lexeme = self:lexeme():sub(2, -2)}
                self:match('<freeform>')
            end

            if self:token() == 'implementation' then
                self:match()
                fsm.implementation = {line = self:line(), lexeme = self:lexeme():sub(2, -2)}
                self:match('<freeform>')
            end

            self:match('fsm')
            fsm.id = self:lexeme()
            fsm.line = self:line()
            self:match('<id>')
            fsm.parameters = self:parseParameters()
            self:expand()
            self:match('{')

            -- Parse the pre and pos actions
            if self:token() == 'before' then
                self:match()
                fsm.before = {line = self:line(), lexeme = self:lexeme():sub(2, -2)}
                self:match('<freeform>')
            end

            if self:token() == 'after' then
                self:match()
                fsm.after = {line = self:line(), lexeme = self:lexeme():sub(2, -2)}
                self:match('<freeform>')
            end

            -- Parse all the states in the fsm
            self:parseState(fsm)

            while self:token() == '<id>' do
                self:parseState(fsm)
            end

            self:match('}')
            self:match('<eof>')

            -- Sort states and transitions by id to allow for better git diffs
            local states = {}

            for _, state in pairs(fsm.states) do
                states[#states + 1] = state
                states[state.id] = state

                local transitions = {}

                for _, transition in pairs(state.transitions) do
                    transitions[#transitions + 1] = transition
                    transitions[transition.id] = transition
                end

                table.sort(transitions, function(e1, e2) return e1.id < e2.id end)
                state.transitions = transitions
            end

            table.sort(states, function(e1, e2) return e1.id < e2.id end)
            fsm.states = states

            return fsm
        end,

        parseState = function(self, fsm)
            local state = {transitions = {}, id = self:lexeme(), line = self:line()}
            self:match('<id>')

            if fsm.states[state.id] then
                self:error(state.line, 'duplicated state "%s" in "%s"', state.id, fsm.id)
            end

            fsm.states[state.id] = state

            -- Set the first state as the initial state
            if not fsm.begin then
                fsm.begin = state.id
            end

            -- Parse the state
            self:expand()
            self:match('{')

            if self:token() == 'before' then
                self:match()
                state.before = {line = self:line(), lexeme = self:lexeme():sub(2, -2)}
                self:match('<freeform>')
            end

            if self:token() == 'after' then
                self:match()
                state.after = {line = self:line(), lexeme = self:lexeme():sub(2, -2)}
                self:match('<freeform>')
            end

            while self:token() == '<id>' do
                self:parseTransition(fsm, state)
            end

            self:match('}')
        end,

        parseTransition = function(self, fsm, state)
            -- Get the transition id
            local transition = {id = self:lexeme(), line = self:line()}
            self:match('<id>')

            if state.transitions[transition.id] then
                self:error(transition.line, 'duplicated transition "%s" in "%s"', transition.id, state.id)
            end

            state.transitions[transition.id] = transition
            transition.parameters = self:parseParameters()

            if self:token(3) ~= '(' then
                -- It's a direct transition to another state
                self:match('=>')
                transition.type = 'state'
                transition.target = {id = self:lexeme(), line = self:line()}
                self:match('<id>')

                -- Check for a precondition for the transition
                if self:token() == '<freeform>' then
                    transition.precondition = {
                        line = self:line(),
                        lexeme = self:lexeme():sub(2, -2):gsub('allow', 'return 1'):gsub('forbid', 'return 0')
                    }

                    self:match('<freeform>')
                else
                    self:match(';')
                end
            else
                -- It's a sequence of transitions that'll arrive on another state
                transition.type = 'sequence'

                local steps = {}
                transition.steps = steps

                -- Parse the other transitions in the sequence
                while self:token() == '=>' do
                    self:match()

                    local step = {id = self:lexeme(), line = self:line()}
                    self:match('<id>')

                    step.arguments = self:parseArguments()
                    steps[#steps + 1] = step
                end

                self:match(';')
            end
        end,

        parseParameters = function(self)
            self:match('(')
            local parameters = {}

            if self:token() ~= ')' then
                while true do
                    local parameter = {}
                    parameters[#parameters + 1] = parameter

                    parameter.type = self:lexeme()
                    self:match('<id>')

                    parameter.id = self:lexeme()
                    parameter.line = self:line()
                    self:match('<id>')

                    if self:lexeme() == ')' then
                        break
                    end

                    self:match(',')
                end
            end

            self:match(')')
            return parameters
        end,

        parseArguments = function(self)
            self:match('(')
            local arguments = {}

            if self:token() ~= ')' then
                while true do
                    local argument = {}
                    arguments[#arguments + 1] = argument

                    argument.id = self:lexeme()
                    argument.line = self:line()
                    self:match('<id>')

                    if self:lexeme() == ')' then
                        break
                    end

                    self:match(',')
                end
            end

            self:match(')')
            return arguments
        end
    }

    parser:init()
    return parser
end

-------------------------------------------------------------------------------
-- Parses a state definition
-------------------------------------------------------------------------------
local function parseState(source, path, startLine, fsm, state)
    local parser = newParser(source, path, startLine)
    return parser:parse()
end

local function parseFsm(path)
    local parser = newParser(path)
    return parser:parse()
end

local function validate(fsm, path)
    local function walk(fsm, state, id)
        local transition = state.transitions[id]
  
        if not transition then
            fatal('', 0, '"Unknown transition "%s"', id)
        end
  
        if transition.type == 'state' then
            state = fsm.states[transition.target.id]
  
            if not state then
                fatal('', 0, '"Unknown state "%s"', transition.target.id)
            end
        elseif transition.type == 'sequence' then
            for _, step in ipairs(transition.steps) do
                state = walk(fsm, state, step.id)
            end
        end
  
        return state
    end
  
    for id, state in pairs(fsm.states) do
        for id, transition in pairs(state.transitions) do
            if transition.type == 'sequence' then
                transition.target = walk(fsm, state, transition.id)
            end

            if not fsm.states[transition.target.id] then
                fatal(path, transition.target.line, '"Unknown state "%s"', transition.target.id)
            end
        end
    end
end

local function emit(fsm, path)
    local idn = '    '

    local dir, name = ddlt.split(path)
    path = ddlt.realpath(path)

    -- Collect and order all transitions
    local allTransitions = {}

    do
        local visited = {}

        for _, state in ipairs(fsm.states) do
            for _, transition in ipairs(state.transitions) do
                if not visited[transition.id] then
                    visited[transition.id] = true
                    allTransitions[#allTransitions + 1] = transition
                end
            end
        end
    end

    table.sort(allTransitions, function(e1, e2) return e1.id < e2.id end)

    -- Emit the header
    local out = assert(io.open(ddlt.join(dir, name, 'h'), 'w'))
    local guard = string.format('%s_H__', name:upper())

    out:write('#ifndef ', guard, '\n')
    out:write('#define ', guard, '\n\n')
    out:write('/* Generated with FSM compiler: https://github.com/leiradel/luamods/ddlt */\n\n')

    out:write('#include <stdarg.h>\n\n')

    -- Header free form code
    if fsm.header then
        out:write(fsm.header.lexeme, '\n\n')
    end

    -- The states enumeration
    out:write('/* FSM states */\n')
    out:write('typedef enum {\n')

    for _, state in ipairs(fsm.states) do
        out:write(idn, fsm.id, '_State_', state.id, ',\n')
    end

    out:write('}\n')
    out:write(fsm.id, '_State;\n\n')

    -- Debug printf
    out:write('typedef void (*', fsm.id, '_Vprintf)(void* const ud, char const* const fmt, va_list args);\n\n')

    -- The FSM state
    out:write('/* The FSM */\n')
    out:write('typedef struct {\n')
    out:write(idn, fsm.id, '_State state;\n')

    for _, parameter in ipairs(fsm.parameters) do
        out:write(idn, parameter.type, ' ', parameter.id, ';\n')
    end
    
    out:write('\n')
    out:write('#ifdef DEBUG_FSM\n')
    out:write(idn, '/* Set those after calling ', fsm.id, '_Init when DEBUG_FSM is defined */\n')
    out:write(idn, fsm.id, '_Vprintf vprintf;\n')
    out:write(idn, 'void* vprintf_ud;\n')
    out:write('#endif\n')
    out:write('}\n')
    out:write(fsm.id, '_Context;\n\n')

    -- The initialization function
    out:write('/* Initialization */\n')
    out:write('void ', fsm.id, '_Init(', fsm.id, '_Context* const self')

    for _, parameter in ipairs(fsm.parameters) do
        out:write(', ', parameter.type, ' const ', parameter.id)
    end

    out:write(');\n\n')

    -- Query functions
    out:write('/* Query */\n')
    out:write(fsm.id, '_State ', fsm.id, '_CurrentState(', fsm.id, '_Context const* const self);\n')
    out:write('int ', fsm.id, '_CanTransitionTo(', fsm.id, '_Context const* const self, ', fsm.id, '_State const next);\n\n')

    -- Transitions
    out:write('/* Transitions */\n')

    for _, transition in ipairs(allTransitions) do
        out:write('int ', fsm.id, '_Transition_', transition.id, '(', fsm.id, '_Context* const self')

        for _, parameter in ipairs(transition.parameters) do
            out:write(', ', parameter.type, ' ', parameter.id)
        end

        out:write(');\n')
    end

    out:write('\n')

    -- Debug
    out:write('/* Debug */\n')
    out:write('#ifdef DEBUG_FSM\n')
    out:write('char const* ', fsm.id, '_StateName(', fsm.id, '_State const state);\n')
    out:write('#endif\n\n')

    -- C++
    out:write('#ifdef __cplusplus\n')
    out:write('class ', fsm.id, ' {\n')
    out:write('public:\n')

    out:write(idn, 'enum class State {\n')

    for _, state in ipairs(fsm.states) do
        out:write(idn, idn, state.id, ' = ', fsm.id, '_State_', state.id, ',\n')
    end

    out:write(idn, '};\n\n')

    out:write(idn, fsm.id, '(')

    for i, parameter in ipairs(fsm.parameters) do
        out:write(i == 1 and '' or ', ', parameter.type, ' const ', parameter.id)
    end

    out:write(') { ', fsm.id, '_Init(&_fsm')

    for _, parameter in ipairs(fsm.parameters) do
        out:write(', ', parameter.id)
    end

    out:write('); }\n\n')

    out:write('#ifdef DEBUG_FSM\n')
    out:write(idn, fsm.id, '(')

    for i, parameter in ipairs(fsm.parameters) do
        out:write(i == 1 and '' or ', ', parameter.type, ' const ', parameter.id)
    end

    out:write(', ', fsm.id, '_Vprintf vprintf, void* vprintf_ud) : ', fsm.id, '(')

    for i, parameter in ipairs(fsm.parameters) do
        out:write(i == 1 and '' or ', ', parameter.id)
    end

    out:write(') {\n')
    out:write(idn, idn, '_fsm.vprintf = vprintf;\n')
    out:write(idn, idn, '_fsm.vprintf_ud = vprintf_ud;\n')
    out:write(idn, '}\n')
    out:write('#endif\n\n')

    out:write(idn, 'State currentState() const { return (State)', fsm.id, '_CurrentState(&_fsm); }\n')
    out:write(idn, 'bool canTransitionTo(State const next) const { return ', fsm.id, '_CanTransitionTo(&_fsm, (', fsm.id, '_State)next) != 0; }\n\n')

    for _, transition in ipairs(allTransitions) do
        out:write(idn, 'bool ', transition.id, '(')

        for i, parameter in ipairs(transition.parameters) do
            out:write(i == 1 and '' or ', ', parameter.type, ' ', parameter.id)
        end

        out:write(') { return ', fsm.id, '_Transition_', transition.id, '(&_fsm')

        for _, parameter in ipairs(transition.parameters) do
            out:write(', ', parameter.id)
        end

        out:write(') != 0; }\n')
    end

    out:write('\n')

    out:write('#ifdef DEBUG_FSM\n')
    out:write(idn, 'static char const* stateName(State const state) { return ', fsm.id, '_StateName((', fsm.id, '_State)state); }\n')
    out:write('#endif\n\n')

    out:write('protected:\n')
    out:write(idn, fsm.id, '_Context _fsm;\n')
    out:write('};\n')
    out:write('#endif\n\n')

    -- Finish up
    out:write('#endif /* ', guard, ' */\n')
    out:close()

    -- Emit the code
    local out = assert(io.open(ddlt.join(dir, name, 'c'), 'w'))

    -- Include the necessary headers
    out:write('/* Generated with FSM compiler: https://github.com/leiradel/luamods/ddlt */\n\n')
    out:write('#include "', name, '.h"\n\n')
    out:write('#include <stddef.h>\n\n')

    -- Implementation free form code
    if fsm.implementation then
        out:write(fsm.implementation.lexeme, '\n\n')
    end

    -- Helper printf-like function
    out:write('#ifdef DEBUG_FSM\n')
    out:write('static void fsmprintf(', fsm.id, '_Context* const self, const char* fmt, ...) {\n')
    out:write(idn, 'if (self->vprintf != NULL) {\n')
    out:write(idn, idn, 'va_list args;\n')
    out:write(idn, idn, 'va_start(args, fmt);\n')
    out:write(idn, idn, 'self->vprintf(self->vprintf_ud, fmt, args);\n');
    out:write(idn, idn, 'va_end(args);\n')
    out:write(idn, '}\n')
    out:write('}\n\n')
    out:write('#define PRINTF(...) do { fsmprintf(__VA_ARGS__); } while (0)\n')
    out:write('#else\n')
    out:write('#define PRINTF(...) do {} while (0)\n')
    out:write('#endif\n\n')

    -- The initialization function
    out:write('/* Initialization */\n')
    out:write('void ', fsm.id, '_Init(', fsm.id, '_Context* const self')

    for _, parameter in ipairs(fsm.parameters) do
        out:write(', ', parameter.type, ' const ', parameter.id)
    end

    out:write(') {\n')
    out:write(idn, 'self->state = ', fsm.id, '_State_', fsm.begin, ';\n\n')

    for _, parameter in ipairs(fsm.parameters) do
        out:write(idn, 'self->', parameter.id, ' = ', parameter.id, ';\n')
    end

    out:write('}\n\n')

    -- Query functions
    out:write(fsm.id, '_State ', fsm.id, '_CurrentState(', fsm.id, '_Context const* const self) {\n')
    out:write(idn, 'return self->state;\n')
    out:write('}\n\n')

    out:write('int ', fsm.id, '_CanTransitionTo(', fsm.id, '_Context const* const self, ', fsm.id, '_State const next) {\n')
    out:write(idn, 'switch (self->state) {\n')

    for _, state in ipairs(fsm.states) do
        out:write(idn, idn, 'case ', fsm.id, '_State_', state.id, ':\n')

        if #state.transitions ~= 0 then
            out:write(idn, idn, idn, 'switch (next) {\n')

            local valid, sorted = {}, {}

            for _, newState in pairs(state.transitions) do
                valid[newState.target.id] = true
            end

            for stateId in pairs(valid) do
                sorted[#sorted + 1] = stateId
            end

            table.sort(sorted, function(id1, id2) return id1 < id2 end)

            for _, stateId in ipairs(sorted) do
                out:write(idn, idn, idn, idn, 'case ', fsm.id, '_State_', stateId, ':\n')
            end

            out:write(idn, idn, idn, idn, idn, 'return 1;\n')
            out:write(idn, idn, idn, idn, 'default: break;\n')
            out:write(idn, idn, idn, '}\n')
        end

        out:write(idn, idn, idn, 'break;\n\n')
    end

    out:write(idn, idn, 'default: break;\n')
    out:write(idn, '}\n\n')
    out:write(idn, 'return 0;\n')
    out:write('}\n\n')

    -- Global before event
    out:write('static int global_before(', fsm.id, '_Context* const self) {\n')
    out:write(idn, '(void)self;\n')

    if fsm.before then
        out:write(fsm.before.lexeme, '\n')
    end

    out:write(idn, 'return 1;\n')
    out:write('}\n\n')

    -- State-specific before events
    out:write('static int local_before(', fsm.id, '_Context* const self) {\n')
    out:write(idn, '(void)self;\n')
    local generate = false

    for _, state in ipairs(fsm.states) do
        generate = generate or state.before
    end

    if generate then
        out:write('\n')
        out:write(idn, 'switch (self->state) {\n')

        for _, state in ipairs(fsm.states) do
            if state.before then
                out:write(idn, idn, 'case ', fsm.id, '_State_', state.id, ': {\n')
                out:write(state.before.lexeme, '\n')
                out:write(idn, idn, '}\n')
                out:write(idn, idn, 'break;\n');
            end
        end

        out:write(idn, idn, 'default: break;\n')
        out:write(idn, '}\n\n')
    end

    out:write(idn, 'return 1;\n')
    out:write('}\n\n')

    -- Global after event
    out:write('static void global_after(', fsm.id, '_Context* const self) {\n')
    out:write(idn, '(void)self;\n')

    if fsm.after then
        out:write(fsm.after.lexeme, '\n')
    end

    out:write('}\n\n')

    -- State-specific after events
    out:write('static void local_after(', fsm.id, '_Context* const self) {\n')
    out:write(idn, '(void)self;\n')
    local generate = false

    for _, state in ipairs(fsm.states) do
        generate = generate or state.after
    end

    if generate then
        out:write('\n')
        out:write(idn, 'switch (self->state) {\n')

        for _, state in ipairs(fsm.states) do
            if state.after then
                out:write(idn, idn, 'case ', fsm.id, '_State_', state.id, ': {\n')
                out:write(state.after.lexeme, '\n')
                out:write(idn, idn, '}\n')
                out:write(idn, idn, 'break;\n');
            end
        end

        out:write(idn, idn, 'default: break;\n')
        out:write(idn, '}\n')
    end

    out:write('}\n\n')

    -- Transitions
    for _, transition in ipairs(allTransitions) do
        out:write('int ', fsm.id, '_Transition_', transition.id, '(', fsm.id, '_Context* const self')

        for _, parameter in ipairs(transition.parameters) do
            out:write(', ', parameter.type, ' ', parameter.id)
        end

        out:write(') {\n')
        out:write(idn, 'switch (self->state) {\n')

        local valid, invalid = {}, {}

        for _, state in ipairs(fsm.states) do
            if state.transitions[transition.id] then
                valid[#valid + 1] = state
            else
                invalid[#invalid + 1] = state
            end
        end

        for _, state in ipairs(valid) do
            local transition2 = state.transitions[transition.id]

            out:write(idn, idn, 'case ', fsm.id, '_State_', state.id, ': {\n')
            out:write(idn, idn, idn, 'PRINTF(\n')
            out:write(idn, idn, idn, idn, 'self,\n')
            out:write(idn, idn, idn, idn, '"FSM %s:%u Transitioning from %s to %s via %s",\n')
            out:write(idn, idn, idn, idn, '__FILE__, __LINE__, LifeCycle_StateName(LifeCycle_CurrentState(self)), "', transition2.target.id, '", "', transition.id, '"\n')
            out:write(idn, idn, idn, ');\n\n')
            out:write(idn, idn, idn, 'if (!global_before(self)) {\n')
            out:write(idn, idn, idn, idn, 'PRINTF(\n')
            out:write(idn, idn, idn, idn, idn, 'self,\n')
            out:write(idn, idn, idn, idn, idn, '"FSM %s:%u Failed global precondition while transitioning to %s",\n')
            out:write(idn, idn, idn, idn, idn, '__FILE__, __LINE__, "', transition2.target.id, '"\n')
            out:write(idn, idn, idn, idn, ');\n\n')
            out:write(idn, idn, idn, idn, 'return 0;\n')
            out:write(idn, idn, idn, '}\n\n')
            out:write(idn, idn, idn, 'if (!local_before(self)) {\n')
            out:write(idn, idn, idn, idn, 'PRINTF(\n')
            out:write(idn, idn, idn, idn, idn, 'self,\n')
            out:write(idn, idn, idn, idn, idn, '"FSM %s:%u Failed state precondition while transitioning to %s",\n')
            out:write(idn, idn, idn, idn, idn, '__FILE__, __LINE__, "', transition2.target.id, '"\n')
            out:write(idn, idn, idn, idn, ');\n\n')
            out:write(idn, idn, idn, idn, 'return 0;\n')
            out:write(idn, idn, idn, '}\n\n')

            if transition2.precondition then
                out:write(transition2.precondition.lexeme, '\n')
            end

            if transition2.type == 'state' then
                out:write(idn, idn, idn, 'self->state = ', fsm.id, '_State_', transition2.target.id, ';\n\n')
            elseif transition2.type == 'sequence' then
                local arguments = function(args)
                    local list = {'self'}

                    for _, arg in ipairs(args) do
                        list[#list + 1] = arg.id
                    end

                    return table.concat(list, ', ')
                end

                if #transition2.steps == 1 then
                    local args = arguments(transition2.steps[1].arguments)
                    out:write(idn, idn, idn, 'const int ok__ = ', fsm.id, '_Transition_', transition2.steps[1].id, '(', args, ');\n')
                else
                    local args = arguments(transition2.steps[1].arguments)
                    out:write(idn, idn, idn, 'const int ok__ = ', fsm.id, '_Transition_', transition2.steps[1].id, '(', args, ') &&\n')

                    for i = 2, #transition2.steps do
                        local args = arguments(transition2.steps[i].arguments)
                        local eol = i == #transition2.steps and ';' or ' &&'
                        out:write(idn, idn, idn, '                 ', fsm.id, '_Transition_', transition2.steps[i].id, '(', args, ')', eol, '\n')
                    end

                    out:write('\n')
                end
            end

            if transition2.type == 'state' then
                out:write(idn, idn, idn, 'local_after(self);\n')
                out:write(idn, idn, idn, 'global_after(self);\n\n')
                out:write(idn, idn, idn, 'PRINTF(\n')
                out:write(idn, idn, idn, idn, 'self,\n')
                out:write(idn, idn, idn, idn, '"FSM %s:%u Transitioned to %s",\n')
                out:write(idn, idn, idn, idn, '__FILE__, __LINE__, "', transition2.target.id, '"\n')
                out:write(idn, idn, idn, ');\n\n')
                out:write(idn, idn, idn, 'return 1;\n')
            elseif transition2.type == 'sequence' then
                out:write(idn, idn, idn, 'if (ok__) {\n')
                out:write(idn, idn, idn, idn, 'local_after(self);\n')
                out:write(idn, idn, idn, idn, 'global_after(self);\n\n')
                out:write(idn, idn, idn, '}\n')
                out:write(idn, idn, idn, 'else {\n')
                out:write(idn, idn, idn, idn, 'PRINTF(\n')
                out:write(idn, idn, idn, idn, idn, 'self,\n')
                out:write(idn, idn, idn, idn, idn, '"FSM %s:%u Failed to transition to %s",\n');
                out:write(idn, idn, idn, idn, idn, '__FILE__, __LINE__, "', transition2.target.id, '"\n')
                out:write(idn, idn, idn, idn, ');\n')
                out:write(idn, idn, idn, '}\n\n')
                out:write(idn, idn, idn, 'return ok__;\n')
            end

            out:write(idn, idn, '}\n\n')
            out:write(idn, idn, 'break;\n\n')
        end

        out:write(idn, idn, 'default: break;\n')
        out:write(idn, '}\n\n')
        out:write(idn, 'PRINTF(\n')
        out:write(idn, idn, 'self,\n')
        out:write(idn, idn, '"FSM %s:%u Transition %s is invalid from state %s",\n');
        out:write(idn, idn, '__FILE__, __LINE__, "', transition.id, '", LifeCycle_StateName(self->state)\n')
        out:write(idn, ');\n\n')
        out:write(idn, 'return 0;\n')
        out:write('}\n\n')
    end

    -- State name
    out:write('#ifdef DEBUG_FSM\n')
    out:write('const char* ', fsm.id, '_StateName(', fsm.id, '_State const state) {\n')
    out:write(idn, 'switch (state) {\n')

    for _, state in ipairs(fsm.states) do
        out:write(idn, idn, 'case ', fsm.id, '_State_', state.id, ': return "', state.id, '";\n')
    end

    out:write(idn, idn, 'default: break;\n')
    out:write(idn, '}\n\n')
    out:write(idn, 'return "unknown state";\n')
    out:write('}\n')
    out:write('#endif\n')

    out:close()

    -- Emit a Graphviz file
    local out = assert(io.open(ddlt.join(dir, name, 'dot'), 'w'))

    out:write('// Generated with FSM compiler, https://github.com/leiradel/luamods/ddlt\n\n')
    out:write('digraph ', fsm.id, ' {\n')

    for _, state in ipairs(fsm.states) do
        out:write(idn, state.id, ' [label="', state.id, '"];\n')
    end

    out:write('\n')

    for _, state in ipairs(fsm.states) do
        for _, transition in ipairs(state.transitions) do
            if transition.type == 'state' then
                out:write(idn, state.id, ' -> ', transition.target.id, ' [label="', transition.id, '"];\n')
            end
        end
    end

    out:write('}\n')
    out:close()
end

if #arg ~= 1 then
    io.stderr:write('Usage: lua fsmc.lua inputfile\n')
    os.exit(1)
end

local fsm = parseFsm(arg[1])
validate(fsm, arg[1])
emit(fsm, arg[1])
