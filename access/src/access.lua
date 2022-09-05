local proxyud = require 'proxyud'

local proxyud_new = proxyud.new
local proxyud_get = proxyud.get
local pairs = pairs
local tostring = tostring

-- The metatable for readonly tables.
local constmt = {
    __index = function(ud, key)
        -- Get the table for the userdata.
        local table = proxyud_get(ud)
        -- Get the value in the table.
        local value = table[key]

        -- Return the value if there's one.
        if value ~= nil then
            return value
        end

        -- Unknown key.
        error(string.format('cannot access unknown field \'%s\' in table', key))
    end,

    __newindex = function(ud, key, value)
        -- Const tables don't support assignments.
        error(string.format('cannot assign to field \'%s\' in constant table', key))
    end,

    -- Forward all other metamethods to the original value
    __add = function(ud, other)    return proxyud_get(ud) + other end,
    __band = function(ud, other)   return proxyud_get(ud) & other end,
    __bnot = function(ud)          return ~proxyud_get(ud) end,
    __bor = function(ud, other)    return proxyud_get(ud) | other end,
    __bxor = function(ud, other)   return proxyud_get(ud) ~ other end,
    __call = function(ud, ...)     return proxyud_get(ud)(...) end,
    __concat = function(ud, other) return proxyud_get(ud) .. other end,
    __div = function(ud, other)    return proxyud_get(ud) / other end,
    __eq = function(ud, other)     return proxyud_get(ud) == other end,
    -- __gc: supposed to be called when the original table is collected
    __idiv = function(ud, other)   return proxyud_get(ud) // other end,
    __le = function(ud, other)     return proxyud_get(ud) <= other end,
    __len = function(ud)           return #proxyud_get(ud) end,
    __lt = function(ud, other)     return proxyud_get(ud) < other end,
    -- __metatable: can't set the this field without duplicating this entire metatable for each table
    __mod = function(ud, other)    return proxyud_get(ud) % other end,
    -- __mode: can't set the this field without duplicating this entire metatable for each table
    __mul = function(ud, other)    return proxyud_get(ud) * other end,
    -- __name: unused?
    __pairs = function(ud)         return pairs(proxyud_get(ud)) end,
    __pow = function(ud, other)    return proxyud_get(ud) ^ other end,
    __shl = function(ud, other)    return proxyud_get(ud) << other end,
    __shr = function(ud, other)    return proxyud_get(ud) >> other end,
    __sub = function(ud, other)    return proxyud_get(ud) - other end,
    __tostring = function(ud)      return tostring(proxyud_get(ud)) end,
    __unm = function(ud)           return -proxyud_get(ud) end
}

-- The metatable for sealed tables.
local sealmt = {}

for key, value in pairs(constmt) do
    sealmt[key] = value
end

sealmt.__newindex = function(ud, key, value)
    -- Get the table for the userdata.
    local table = proxyud_get(ud)

    -- Only assign if the field exists.
    if table[key] ~= nil then
        table[key] = value
        return
    end

    -- Unknown key.
    error(string.format('cannot assign to unknown field \'%s\' in sealed table', key))
end

-- The metatable for record tables.
local recordmt = {}

for key, value in pairs(constmt) do
    recordmt[key] = value
end

recordmt.__newindex = function(ud, key, value)
    -- Get the table for the userdata.
    local table = proxyud_get(ud)

    -- Only assign if the field exists and the values's types are the same.
    local oldvalue = table[key]

    if oldvalue ~= nil then
        if type(oldvalue) == type(value) then
            table[key] = value
            return
        end

        -- Wrong type.
        error(string.format('cannot assign to field \'%s\' of different type in record table', key))
    end

    -- Unknown key.
    error(string.format('cannot assign to unknown field \'%s\' in record table', key))
end

local function const(table)
    -- Create and return an userdata with the table and the constmt metatable.
    return proxyud_new(table, constmt)
end

local function seal(table)
    return proxyud_new(table, sealmt)
end

local function record(table)
    return proxyud_new(table, recordmt)
end

-- The module.
return const {
    _COPYRIGHT = 'Copyright (c) 2021-2022 Andre Leiradella',
    _LICENSE = 'MIT',
    _VERSION = '1.2.0',
    _NAME = 'access',
    _URL = 'https://github.com/leiradel/luamods/access',
    _DESCRIPTION = 'Creates constant and sealed objects',

    const = const,
    seal = seal,
    record = record
}
