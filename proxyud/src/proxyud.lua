return function(M)
    local proxyud_get = M.get
    local getmetatable = getmetatable
    local pairs = pairs
    local tostring = tostring

    M.identitymetatable = function(get)
        get = get or proxyud_get

        return {
            __add = function(ud, other)    return get(ud) + other end,
            __band = function(ud, other)   return get(ud) & other end,
            __bnot = function(ud)          return ~get(ud) end,
            __bor = function(ud, other)    return get(ud) | other end,
            __bxor = function(ud, other)   return get(ud) ~ other end,
            __call = function(ud, ...)     return get(ud)(...) end,

            __close = function(ud, err)
                local object = get(ud)
                local metatable = getmetatable(object)
        
                if metatable then
                    local metamethod = metatable.__close
        
                    if metamethod then
                        metamethod(object, err)
                    end
                end
            end,

            __concat = function(ud, other) return get(ud) .. other end,
            __div = function(ud, other)    return get(ud) / other end,
            __eq = function(ud, other)     return get(ud) == other end,
            -- __gc
            __idiv = function(ud, other)   return get(ud) // other end,
            __index = function(ud, key)    return get(ud)[key] end,
            __le = function(ud, other)     return get(ud) <= other end,
            __len = function(ud)           return #get(ud) end,
            __lt = function(ud, other)     return get(ud) < other end,
            __metatable = function(ud)     return getmetatable(get(ud)) end,
            __mod = function(ud, other)    return get(ud) % other end,
            -- __mode
            __mul = function(ud, other)    return get(ud) * other end,
            -- __name
            
            __newindex = function(ud, key, value)
                get(ud)[key] = value
            end,

            __pairs = function(ud)         return pairs(get(ud)) end,
            __pow = function(ud, other)    return get(ud) ^ other end,
            __shl = function(ud, other)    return get(ud) << other end,
            __shr = function(ud, other)    return get(ud) >> other end,
            __sub = function(ud, other)    return get(ud) - other end,
            __tostring = function(ud)      return tostring(get(ud)) end,
            __unm = function(ud)           return -get(ud) end
        }
    end
end
