local access = require 'access'

local function expect(expected, func)
    assert(pcall(func) == expected)
end

-------------------------------------------------------------------------------
-- Const

local c = access.const {
    pi = math.pi
}

-- Ok, c.pi exists
expect(true, function() return c.pi end)

-- Throws an error, c.huge doesn't exist.
expect(false, function() return c.huge end)

-- Throws an error, the table is constant.
expect(false, function() c.pi = math.huge end)

-- Throws an error, c.huge doesn't exist (but also the table is constant).
expect(false, function() c.huge = math.huge end)

-------------------------------------------------------------------------------
-- Seal

local s = access.seal {
    pi = math.pi
}

-- Ok, s.pi exists
expect(true, function() return s.pi end)

-- Throws an error, s.huge doesn't exist.
expect(false, function() return s.huge end)

-- Ok, c.pi exists.
expect(true, function() s.pi = math.huge end)

-- Throws an error, s.huge doesn't exist.
expect(false, function() s.huge = math.huge end)

-------------------------------------------------------------------------------
-- Record

local r = access.record {
    pi = math.pi
}

-- Ok, r.pi exists
expect(true, function() return r.pi end)

-- Throws an error, r.huge doesn't exist.
expect(false, function() return r.huge end)

-- Ok, r.pi exists, is a number, and so is math.huge.
expect(true, function() r.pi = math.huge end)

-- Throws an error, r.huge doesn't exist.
expect(false, function() r.huge = math.huge end)

-- Throws an error, r.pi's type is not string.
expect(false, function() r.pi = 'test' end)

-------------------------------------------------------------------------------
-- Metamethods

local collected = false

local mt = {
    __add = function(self, other)    return 1 end,
    __band = function(self, other)   return 2 end,
    __bnot = function(self)          return 3 end,
    __bor = function(self, other)    return 4 end,
    __bxor = function(self, other)   return 5 end,
    __call = function(self, ...)     return 6 end,
    __concat = function(self, other) return 7 end,
    __div = function(self, other)    return 8 end,
    __eq = function(self, other)     return 9 end,
    __gc = function(self)            collected = true end,
    __idiv = function(self, other)   return 10 end,
    __le = function(self, other)     return 11 end,
    __len = function(self)           return 12 end,
    __lt = function(self, other)     return 13 end,
    -- __metatable
    __mod = function(self, other)    return 14 end,
    -- __mode
    __mul = function(self, other)    return 15 end,
    -- __name
    __pairs = function(self)         return next, self, nil end,
    __pow = function(self, other)    return 16 end,
    __shl = function(self, other)    return 17 end,
    __shr = function(self, other)    return 18 end,
    __sub = function(self, other)    return 19 end,
    __tostring = function(self)      return 20 end,
    __unm = function(self)           return 21 end
}

do
    local m = access.const(debug.setmetatable({one = 1}, mt))
    local n = access.const(debug.setmetatable({}, mt))

    assert((m + 0) == 1)
    assert((m & 0) == 2)
    assert((~m) == 3)
    assert((m | 0) == 4)
    assert((m ~ 0) == 5)
    assert((m()) == 6)
    assert((m .. '') == 7)
    assert((m / 0) == 8)
    --assert(m == n) -- not sure how to make __eq work
    assert((m // 0) == 10)
    assert(m <= n)
    assert((#m) == 12)
    assert(m < n)
    assert((m % 0) == 14)
    assert((m * 0) == 15)
    assert((m ^ 0) == 16)
    assert((m << 0) == 17)
    assert((m >> 0) == 18)
    assert((m - 0) == 19)
    assert((tostring(m)) == '20')
    assert((-m) == 21)

    for k, v in pairs(m) do
        assert(k == 'one' and v == 1)
    end
end

collectgarbage('collect')
assert(collected)
