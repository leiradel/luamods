local ddlt = require 'ddlt'

local function dump(t, z, i)
    z = z or {}
    i = i or ''

    if z[t] then
        io.write(string.format('%s"%s (recursive!)",\n', i, tostring(t)))
        return
    end

    z[t] = true
    io.write(string.format('%s{\n', i))

    for k, v in pairs(t) do
        io.write(string.format('%s  "%s" = ', i, tostring(k)))

        if type(v) == 'table' then
            io.write('\n')
            dump(v, z, i .. '    ')
        else
            io.write(string.format('%q,', tostring(v)))
        end

        io.write('\n')
    end

    io.write(string.format('%s},\n', i))
end

local function fatal(path, line, format, ...)
    local location = string.format('%s:%u: ', path, line)
    local message = string.format(format, ...)
    error(string.format('%s%s', location, message))
    io.stderr:write(location, message, '\n')
    os.exit(1)
end

local function log(...)
    io.stderr:write(string.format(...))
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
        symbols = {'=>', '(', ')', ';', ',', ':'},
        keywords = {'header', 'implementation', 'fsm', 'before', 'after', 'stack'},
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

            while self:token() == '<id>' or self:token() == 'stack' do
                self:parseState(fsm)
            end

            self:match('}')
            self:match('<eof>')

            return fsm
        end,

        parseState = function(self, fsm)
            local state = {transitions = {}, id = self:lexeme(), line = self:line()}

            if self:token() == 'stack' then
                self:match('stack')
                state.stack = true
            else
                self:match('<id>')
            end

            if fsm.states[state.id] then
                self:error(state.line, 'duplicated state "%s" in "%s"', state.id, fsm.id)
            end

            fsm.states[state.id] = state

            -- Set the first non-stack state as the initial state
            if not state.stack and not fsm.begin then
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
            -- Get the list of allowed states for the transition
            local allowed = {}

            if self:token(2) ~= '(' then
                -- We have a list of allowed states
                if not state.stack then
                    self:error(self:line(), 'allowed states list can only appear in transition in the "stack" state')
                end

                local id, line = self:lexeme(), self:line()
                self:match('<id>')

                if allowed[id] then
                    self:error(self:line(), 'duplicated state "%s" in allowed states list', id)
                end

                allowed[id] = {id = id, line = self:line()}

                while self:token() == ',' do
                    self:match(',')

                    local id, line = self:lexeme(), self:line()
                    self:match('<id>')
    
                    if allowed[id] then
                        self:error(self:line(), 'duplicated state "%s" in allowed states list', id)
                    end
    
                    allowed[id] = {id = id, line = self:line()}
                end

                self:match(':')
            end

            -- Get the transition id
            local transition = {id = self:lexeme(), line = self:line()}
            self:match('<id>')

            if next(allowed) then
                transition.allowed = allowed
            end

            if state.transitions[transition.id] then
                self:error(transition.line, 'duplicated transition "%s" in "%s"', transition.id, state.id)
            end

            state.transitions[transition.id] = transition
            transition.parameters = self:parseParameters()

            if self:token() ~= '=>' then
                -- It's a pop, check for a precondition for the transition
                if not state.stack then
                    self:error(self:line(), '"pop" transitions are only allowed in the "stack" state')
                end

                transition.type = 'pop'

                if self:token() == '<freeform>' then
                    transition.precondition = {
                        line = self:line(),
                        lexeme = self:lexeme():sub(2, -2):gsub('allow', 'return 1'):gsub('forbid', 'return 0')
                    }

                    self:match('<freeform>')
                else
                    self:match(';')
                end
            elseif self:token(3) ~= '(' then
                -- It's a direct transition to another state
                transition.type = state.stack and 'push' or 'state'
                self:match('=>')
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
                if state.stack then
                    self:error(self:line(), 'sequenced transitions are not allowed in the "stack" state')
                end

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
    local fsm = parser:parse()

    -- Validate the FSM
    local function walk(fsm, state, id)
        local transition = state.transitions[id]
  
        if not transition then
            fatal(path, 0, '"Unknown transition "%s"', id)
        end
  
        if transition.type == 'state' then
            state = fsm.states[transition.target.id]
  
            if not state then
                fatal(path, transition.target.line, '"Unknown state "%s"', transition.target.id)
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
            if state.stack then
                -- Check that states in allowed lists exist
                if transition.allowed then
                    for _, allowed in pairs(transition.allowed) do
                        if not fsm.states[allowed.id] then
                            fatal(path, allowed.line, '"Unknown state "%s"', allowed.id)
                        end
                    end
                end

                -- Each push transition must have a corresponding pop one where
                -- the target state of the former appears in the allowed list
                -- of exactly one of the later
                if transition.type == 'push' then
                    local found, pop = 0, nil

                    for _, transition2 in pairs(state.transitions) do
                        if transition2.type == 'pop' then
                            if transition2.allowed and transition2.allowed[transition.target.id] then
                                found = found + 1
                                pop = transition2
                            end
                        end
                    end

                    if found == 0 then
                        fatal(
                            path, transition.line,
                            'Target state "%s" does not appear in any allowed list of pop transitions',
                            transition.target.id
                        )
                    elseif found > 1 then
                        fatal(
                            path, transition.line,
                            'Target state "%s" appears in multiple allowed list of pop transitions',
                            transition.target.id
                        )
                    end

                    -- Save the pop transition in the push one to use below
                    transition.pop = pop
                end
            else
                -- Check that all intermediary transitions and states in
                -- sequenced transitions exist
                if transition.type == 'sequence' then
                    transition.target = walk(fsm, state, transition.id)
                end

                -- Check that target states exist
                if not fsm.states[transition.target.id] then
                    fatal(path, transition.target.line, '"Unknown state "%s"', transition.target.id)
                end
            end
        end
    end

    -- Add stacked transitions to all allowed states
    local function clone(src, dest)
        for k, v in pairs(src) do
            if type(v) == 'table' then
                dest[k] = clone(v, {})
            else
                dest[k] = v
            end
        end

        return dest
    end

    -- Build an array of states (iterating over fsm.states with pairs was
    -- causing an weird bug)
    local states = {}

    for _, state in pairs(fsm.states) do
        states[#states + 1] = state
    end

    local function processPush(push)
        for _, state in ipairs(states) do
            if not state.stack then
                if push.target.id == state.id then
                    -- Do not add the transition to its own target state
                    log('    Transition target is this state, not adding\n')
                    add = false
                elseif push.allowed == nil or push.allowed[state.id] then
                    -- No need to clone
                    state.transitions[push.id] = push
                    log('    Added transition %s (%s) to state %s\n', push.id, push.type, state.id)
                end
            end
        end

        -- Add the pop transition
        local list = push.allowed or fsm.states

        for id in pairs(list) do
            if id ~= 'stack' and id ~= push.target.id then
                -- The transition will be overridden over and over here, but we
                -- only need one; the correct code to pop to the previous state
                -- will be generated
                local pop = clone(push.pop, {})
                pop.target = {id = id, line = pop.line}
                local state = fsm.states[push.target.id]
                state.transitions[pop.id] = pop

                log('    Added pop transition %s from %s to %s\n', pop.id, push.target.id, pop.target.id)
            end
        end
    end

    -- Process push transitions
    for _, state in ipairs(states) do
        if state.stack then
            -- The stack state, add direct transitions to all the allowed states
            for _, transition in pairs(state.transitions) do
                if transition.type == 'push' then
                    log('Adding push transition %s\n', transition.id)
                    processPush(transition)
                end
            end
        end
    end

    -- Sort states and transitions by id to allow for better git diffs
    for _, state in ipairs(states) do
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
end

local function emit(fsm, path)
    local dir, name = ddlt.split(path)
    path = ddlt.realpath(path)

    -- Create the writer object
    local out = {
        i = 0,

        indent = function(self)
            self.i = self.i + 1
        end,

        unindent = function(self)
            self.i = self.i - 1
        end
    }

    out = setmetatable(out, {
        __call = function(self, ...)
            self.file:write(('    '):rep(self.i))
            self.file:write(...)
            self.file:write('\n')
        end
    })

    -- Collect and order all transitions
    local allTransitions = {}

    do
        local visited = {}

        for _, state in ipairs(fsm.states) do
            for _, transition in ipairs(state.transitions) do
                if not state.stack or (state.stack and transition.type == 'pop') then
                    if not visited[transition.id] then
                        visited[transition.id] = true
                        allTransitions[#allTransitions + 1] = transition
                    end
                end
            end
        end
    end

    table.sort(allTransitions, function(e1, e2) return e1.id < e2.id end)

    -- Emit the header
    out.file = assert(io.open(ddlt.join(dir, name, 'h'), 'w'))
    local guard = string.format('%s_H__', name:upper())

    out('#ifndef ', guard)
    out('#define ', guard, '\n')
    out('/* Generated with FSM compiler: https://github.com/leiradel/luamods/ddlt */\n')

    out('#include <stdarg.h>\n')

    -- Header free form code
    if fsm.header then
        out(fsm.header.lexeme, '\n')
    end

    -- The states enumeration
    out('/* FSM states */')
    out('typedef enum {')
    out:indent()

    for _, state in ipairs(fsm.states) do
        if not state.stack then
            out(fsm.id, '_State_', state.id, ',')
        end
    end

    out:unindent()
    out('}')
    out(fsm.id, '_State;\n')

    -- The FSM state
    out('#define FSM_STACK 16\n')
    out('/* The FSM */')
    out('typedef struct {')
    out:indent()

    out(fsm.id, '_State state[FSM_STACK];')
    out('int sp;\n');

    for _, parameter in ipairs(fsm.parameters) do
        out(parameter.type, ' ', parameter.id, ';')
    end
    
    out('')
    out:unindent()
    out('#ifdef DEBUG_FSM')
    out:indent()

    out('/* Set those after calling ', fsm.id, '_Init when DEBUG_FSM is defined */')
    out('void (*vprintf)(void* const ud, char const* const fmt, va_list args);')
    out('void* vprintf_ud;')

    out:unindent()
    out('#endif')
    out('}')
    out(fsm.id, '_Context;\n')

    -- The initialization function
    out('/* Initialization */')
    do
        local str = {'void ', fsm.id, '_Init(', fsm.id, '_Context* const self'}

        for _, parameter in ipairs(fsm.parameters) do
            str[#str + 1] = ', '
            str[#str + 1] = parameter.type
            str[#str + 1] = ' const '
            str[#str + 1] = parameter.id
        end

        out(table.concat(str, ''), ');\n')
    end

    -- Query functions
    out('/* Query */')
    out(fsm.id, '_State ', fsm.id, '_CurrentState(', fsm.id, '_Context const* const self);')
    out('int ', fsm.id, '_CanTransitionTo(', fsm.id, '_Context const* const self, ', fsm.id, '_State const next);\n')

    -- Transitions
    out('/* Transitions */')

    for _, transition in ipairs(allTransitions) do
        local str = {'int ', fsm.id, '_Transition_', transition.id, '(', fsm.id, '_Context* const self'}

        for _, parameter in ipairs(transition.parameters) do
            str[#str + 1] = ', '
            str[#str + 1] = parameter.type
            str[#str + 1] = ' const '
            str[#str + 1] = parameter.id
        end

        out(table.concat(str, ''), ');')
    end

    out('')

    -- Debug
    out('/* Debug */')
    out('#ifdef DEBUG_FSM')
    out('char const* ', fsm.id, '_StateName(', fsm.id, '_State const state);')
    out('#endif\n')

    -- Finish up
    out('#endif /* ', guard, ' */')
    out.file:close()

    -- Emit the code
    out.file = assert(io.open(ddlt.join(dir, name, 'c'), 'w'))

    -- Include the necessary headers
    out('/* Generated with FSM compiler: https://github.com/leiradel/luamods/ddlt */\n')
    out('#include "', name, '.h"\n')
    out('#include <stddef.h>\n')

    -- Implementation free form code
    if fsm.implementation then
        out(fsm.implementation.lexeme, '\n')
    end

    -- Helper printf-like function
    out('#ifdef DEBUG_FSM\n')
    out('static void fsmprintf(', fsm.id, '_Context* const self, const char* fmt, ...) {')
    out:indent()
    out('if (self->vprintf != NULL) {')
    out:indent()
    out('va_list args;')
    out('va_start(args, fmt);')
    out('self->vprintf(self->vprintf_ud, fmt, args);');
    out('va_end(args);')
    out:unindent()
    out('}')
    out:unindent()
    out('}\n')

    out('#define PRINTF(...) do { fsmprintf(__VA_ARGS__); } while (0)')
    out('#else')
    out('#define PRINTF(...) do {} while (0)')
    out('#endif\n')

    -- The initialization function
    out('/* Initialization */')

    do
        local str = {'void ', fsm.id, '_Init(', fsm.id, '_Context* const self'}

        for _, parameter in ipairs(fsm.parameters) do
            str[#str + 1] = ', '
            str[#str + 1] = parameter.type
            str[#str + 1] = ' const '
            str[#str + 1] = parameter.id
        end

        out(table.concat(str, ''), ') {')
    end

    out:indent()
    out('self->state[0] = ', fsm.id, '_State_', fsm.begin, ';')
    out('self->sp = 0;\n')

    for _, parameter in ipairs(fsm.parameters) do
        out('self->', parameter.id, ' = ', parameter.id, ';')
    end

    out:unindent()
    out('}\n')

    -- Query functions
    out(fsm.id, '_State ', fsm.id, '_CurrentState(', fsm.id, '_Context const* const self) {')
    out:indent()
    out('return self->state[self->sp];')
    out:unindent()
    out('}\n')

    out('int ', fsm.id, '_CanTransitionTo(', fsm.id, '_Context const* const self, ', fsm.id, '_State const next) {')
    out:indent()
    out('switch (self->state[self->sp]) {')
    out:indent()

    for _, state in ipairs(fsm.states) do
        if not state.stack then
            local valid, sorted = {}, {}

            for _, transition in ipairs(state.transitions) do
                if transition.type ~= 'pop' then
                    valid[transition.target.id] = true
                end
            end

            for stateId in pairs(valid) do
                sorted[#sorted + 1] = stateId
            end

            table.sort(sorted, function(id1, id2) return id1 < id2 end)

            if #sorted ~= 0 then
                out('case ', fsm.id, '_State_', state.id, ':')
                out:indent()
    
                out('switch (next) {')
                out:indent()

                for _, stateId in ipairs(sorted) do
                    out('case ', fsm.id, '_State_', stateId, ':')
                end

                out:indent()
                out('return 1;')
                out:unindent()

                out('default: break;')

                out:unindent()
                out('}')
                out('break;\n')
                out:unindent()
            end
        else
            for _, transition in ipairs(state.transitions) do
                if transition.type == 'pop' then
                    for _, allowed in pairs(transition.allowed) do
                        out('case ', fsm.id, '_State_', allowed.id, ':')
                        out:indent()
            
                        out('switch (next) {')
                        out:indent()

                        for _, transition2 in ipairs(state.transitions) do
                            if transition2.type == 'push' and transition2.target.id == allowed.id then
                                local list = transition2.allowed or fsm.states

                                for _, item in ipairs(list) do
                                    if item.id ~= 'stack' then
                                        out('case ', fsm.id, '_State_', item.id, ':')
                                    end
                                end

                                out:indent()
                                out('return 1;')
                                out:unindent()
                            end
                        end

                        out:unindent()
                        out('}')
                        out('break;\n')
                        out:unindent()
                    end
                end
            end
        end
    end

    out('default: break;')
    out:unindent()
    out('}\n')
    out('return 0;')

    out:unindent()
    out('}\n')

    -- Global before event
    out('static int global_before(', fsm.id, '_Context* const self) {')
    out:indent()
    out('(void)self;')

    if fsm.before then
        out(fsm.before.lexeme)
    end

    out('return 1;')

    out:unindent()
    out('}\n')

    -- State-specific before events
    out('static int local_before(', fsm.id, '_Context* const self) {')
    out:indent()

    out('switch (self->state[self->sp]) {')
    out:indent()

    for _, state in ipairs(fsm.states) do
        if state.before then
            out('case ', fsm.id, '_State_', state.id, ': {')
            out:indent()
            out(state.before.lexeme)
            out:unindent()
            out('}')
            out('break;');
        end
    end

    out('default: break;')

    out:unindent()
    out('}\n')
    out('return 1;')

    out:unindent()
    out('}\n')

    -- Global after event
    out('static void global_after(', fsm.id, '_Context* const self) {')
    out:indent()

    out('(void)self;')

    if fsm.after then
        out(fsm.after.lexeme, '\n')
    end

    out:unindent()
    out('}\n')

    -- State-specific after events
    out('static void local_after(', fsm.id, '_Context* const self) {')
    out:indent()

    out('switch (self->state[self->sp]) {')
    out:indent()

    for _, state in ipairs(fsm.states) do
        if state.after then
            out('case ', fsm.id, '_State_', state.id, ': {')
            out:indent()
            out(state.after.lexeme)
            out:unindent()
            out('}')
            out('break;');
        end
    end

    out('default: break;')

    out:unindent()
    out('}')

    out:unindent()
    out('}\n')

    -- Transitions
    local function clone(src, dest)
        for k, v in pairs(src) do
            if type(v) == 'table' then
                dest[k] = clone(v, {})
            else
                dest[k] = v
            end
        end

        return dest
    end

    for _, transition in ipairs(allTransitions) do
        log('Generating code for transition %s (%s)\n', transition.id, transition.type)
        do
            local str = {'int ', fsm.id, '_Transition_', transition.id, '(', fsm.id, '_Context* const self'}

            for _, parameter in ipairs(transition.parameters) do
                str[#str + 1] = ', '
                str[#str + 1] = parameter.type
                str[#str + 1] = ' const '
                str[#str + 1] = parameter.id
            end

            out(table.concat(str, ''), ') {')
        end

        out:indent()
        out('switch (self->state[self->sp]) {')
        out:indent()

        local valid, invalid = {}, {}

        for _, state in ipairs(fsm.states) do
            if state.transitions[transition.id] then
                log('    Transition is valid from state %s\n', state.id)
                valid[#valid + 1] = state
            else
                log('    Transition is NOT valid from state %s\n', state.id)
                invalid[#invalid + 1] = state
            end
        end

        local function dotransition(state, transition2)
            log('    Generating case for transition %s (%s) from state %s\n', transition2.id, transition2.type, state.id)
            out('case ', fsm.id, '_State_', state.id, ': {')
            out:indent()

            if transition2.type == 'pop' then
                out('if (self->sp == 0) {')
                out:indent()

                out('PRINTF(self, "FSM %s:%u Stack underflow while popping the state", __FILE__, __LINE__);')
                out('return 0;')

                out:unindent()
                out('}\n')
            elseif transition2.type == 'push' then
                if transition2.stack then
                    out('if (self->sp == (FSM_STACK - 1)) {')
                    out:indent()

                    out('PRINTF(self, "FSM %s:%u Stack overflow while switching to %s", __FILE__, __LINE__, "', transition2.target.id, '");')
                    out('return 0;')

                    out:unindent()
                    out('}\n')
                end
            end
            
            out('if (!global_before(self)) {')
            out:indent()

            if transition2.type == 'pop' then
                out('PRINTF(self, "FSM %s:%u Failed global precondition while switching to %s", __FILE__, __LINE__, ', fsm.id, '_StateName(self->state[self->sp - 1]));')
            else
                out('PRINTF(self, "FSM %s:%u Failed global precondition while switching to %s", __FILE__, __LINE__, "', transition2.target.id, '");')
            end

            out('return 0;')

            out:unindent()
            out('}\n')

            out('if (!local_before(self)) {')
            out:indent()

            if transition2.type == 'pop' then
                out('PRINTF(self, "FSM %s:%u Failed global precondition while switching to %s", __FILE__, __LINE__, ', fsm.id, '_StateName(self->state[self->sp - 1]));')
            else
                out('PRINTF(self, "FSM %s:%u Failed state precondition while switching to %s", __FILE__, __LINE__, "', transition2.target.id, '");')
            end

            out('return 0;')

            out:unindent()
            out('}\n')

            if transition2.precondition then
                out(transition2.precondition.lexeme, '\n')
            end

            if transition2.type == 'pop' then
                out('self->sp--;\n')
            elseif transition2.type == 'push' then
                out('self->sp++;')
                out('self->state[self->sp] = ', fsm.id, '_State_', transition2.target.id, ';\n')
            elseif transition2.type == 'state' then
                out('self->state[self->sp] = ', fsm.id, '_State_', transition2.target.id, ';\n')
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
                    out('const int ok__ = ', fsm.id, '_Transition_', transition2.steps[1].id, '(', args, ');')
                else
                    local args = arguments(transition2.steps[1].arguments)
                    out('const int ok__ = ', fsm.id, '_Transition_', transition2.steps[1].id, '(', args, ') &&')

                    for i = 2, #transition2.steps do
                        local args = arguments(transition2.steps[i].arguments)
                        local eol = i == #transition2.steps and ';' or ' &&'
                        out('                 ', fsm.id, '_Transition_', transition2.steps[i].id, '(', args, ')', eol)
                    end

                    out('')
                end
            end

            if transition2.type == 'state' or transition2.type == 'pop' then
                out('local_after(self);')
                out('global_after(self);\n')

                if transition2.type == 'pop' then
                    out('PRINTF(self, "FSM %s:%u State popped to %s", __FILE__, __LINE__, ', fsm.id, '_StateName(self->state[self->sp]));')
                else
                    out('PRINTF(self, "FSM %s:%u Switched to %s", __FILE__, __LINE__, "', transition2.target.id, '");')
                end

                out('return 1;\n')
            elseif transition2.type == 'sequence' then
                out('if (ok__) {')
                out:indent()

                out('local_after(self);')
                out('global_after(self);')

                out:unindent()
                out('}')

                out('else {')
                out:indent()

                out('PRINTF(self, "FSM %s:%u Failed to switch to %s", __FILE__, __LINE__, "', transition2.target.id, '");')
                
                out:unindent()
                out('}\n')
                out('return ok__;')
            end

            out:unindent()
            out('}\n')
            out('break;\n')
        end

        for _, state in ipairs(valid) do
            if not state.stack then
                dotransition(state, state.transitions[transition.id])
            end
        end

        out('default: break;')

        out:unindent()
        out('}\n')
        out('return 0;')

        out:unindent()
        out('}\n')
    end

    -- State name
    out('#ifdef DEBUG_FSM')
    out('const char* ', fsm.id, '_StateName(', fsm.id, '_State const state) {')
    out:indent()

    out('switch (state) {')
    out:indent()

    for _, state in ipairs(fsm.states) do
        if not state.stack then
            out('case ', fsm.id, '_State_', state.id, ': return "', state.id, '";')
        end
    end

    out('default: break;')

    out:unindent()
    out('}\n')

    out('return "unknown state";')

    out:unindent()
    out('}')
    out('#endif')

    out.file:close()

    -- Emit a Graphviz file
    out.file = assert(io.open(ddlt.join(dir, name, 'dot'), 'w'))

    out('// Generated with FSM compiler, https://github.com/leiradel/luamods/ddlt\n')
    out('digraph ', fsm.id, ' {')
    out:indent()

    for _, state in ipairs(fsm.states) do
        if not state.stack then
            out(state.id, ' [label="', state.id, '"];')
        end
    end

    out('')

    for _, state in ipairs(fsm.states) do
        if not state.stack then
            for _, transition in ipairs(state.transitions) do
                if transition.type == 'state' or transition.type == 'pop' then
                    out(state.id, ' -> ', transition.target.id, ' [label="', transition.id, '"];')
                end
            end
        end
    end

    out:unindent()
    out('}')
    out.file:close()
end

if #arg ~= 1 then
    io.stderr:write('Usage: lua fsmc.lua inputfile\n')
    os.exit(1)
end

local fsm = parseFsm(arg[1])
emit(fsm, arg[1])
