local jdesc = require 'jdesc'

local names = { [0] =
    "nop",          "aconst_null",     "iconst_m1",     "iconst_0",
    "iconst_1",     "iconst_2",        "iconst_3",      "iconst_4",
    "iconst_5",     "lconst_0",        "lconst_1",      "fconst_0",
    "fconst_1",     "fconst_2",        "dconst_0",      "dconst_1",
    "bipush",       "sipush",          "ldc",           "ldc_w",
    "ldc2_w",       "iload",           "lload",         "fload",
    "dload",        "aload",           "iload_0",       "iload_1",
    "iload_2",      "iload_3",         "lload_0",       "lload_1",
    "lload_2",      "lload_3",         "fload_0",       "fload_1",
    "fload_2",      "fload_3",         "dload_0",       "dload_1",
    "dload_2",      "dload_3",         "aload_0",       "aload_1",
    "aload_2",      "aload_3",         "iaload",        "laload",
    "faload",       "daload",          "aaload",        "baload",
    "caload",       "saload",          "istore",        "lstore",
    "fstore",       "dstore",          "astore",        "istore_0",
    "istore_1",     "istore_2",        "istore_3",      "lstore_0",
    "lstore_1",     "lstore_2",        "lstore_3",      "fstore_0",
    "fstore_1",     "fstore_2",        "fstore_3",      "dstore_0",
    "dstore_1",     "dstore_2",        "dstore_3",      "astore_0",
    "astore_1",     "astore_2",        "astore_3",      "iastore",
    "lastore",      "fastore",         "dastore",       "aastore",
    "bastore",      "castore",         "sastore",       "pop",
    "pop2",         "dup",             "dup_x1",        "dup_x2",
    "dup2",         "dup2_x1",         "dup2_x2",       "swap",
    "iadd",         "ladd",            "fadd",          "dadd",
    "isub",         "lsub",            "fsub",          "dsub",
    "imul",         "lmul",            "fmul",          "dmul",
    "idiv",         "ldiv",            "fdiv",          "ddiv",
    "irem",         "lrem",            "frem",          "drem",
    "ineg",         "lneg",            "fneg",          "dneg",
    "ishl",         "lshl",            "ishr",          "lshr",
    "iushr",        "lushr",           "iand",          "land",
    "ior",          "lor",             "ixor",          "lxor",
    "iinc",         "i2l",             "i2f",           "i2d",
    "l2i",          "l2f",             "l2d",           "f2i",
    "f2l",          "f2d",             "d2i",           "d2l",
    "d2f",          "i2b",             "i2c",           "i2s",
    "lcmp",         "fcmpl",           "fcmpg",         "dcmpl",
    "dcmpg",        "ifeq",            "ifne",          "iflt",
    "ifge",         "ifgt",            "ifle",          "if_icmpeq",
    "if_icmpne",    "if_icmplt",       "if_icmpge",     "if_icmpgt",
    "if_icmple",    "if_acmpeq",       "if_acmpne",     "goto",
    "jsr",          "ret",             "tableswitch",   "lookupswitch",
    "ireturn",      "lreturn",         "freturn",       "dreturn",
    "areturn",      "return",          "getstatic",     "putstatic",
    "getfield",     "putfield",        "invokevirtual", "invokespecial",
    "invokestatic", "invokeinterface", nil --[[ba]],    "new",
    "newarray",     "anewarray",       "arraylength",   "athrow",
    "checkcast",    "instanceof",      "monitorenter",  "monitorexit",
    "wide",         "multianewarray",  "ifnull",        "ifnonnull",
    "goto_w",       "jsr_w",           nil --[[ca]],    nil --[[cb]],
    nil --[[cc]],   nil --[[cd]],      nil --[[ce]],    nil --[[cf]],
    nil --[[d0]],   nil --[[d1]],      nil --[[d2]],    nil --[[d3]],
    nil --[[d4]],   nil --[[d5]],      nil --[[d6]],    nil --[[d7]],
    nil --[[d8]],   nil --[[d9]],      nil --[[da]],    nil --[[db]],
    nil --[[dc]],   nil --[[dd]],      nil --[[de]],    nil --[[df]],
    nil --[[e0]],   nil --[[e1]],      nil --[[e2]],    nil --[[e3]],
    nil --[[e4]],   nil --[[e5]],      nil --[[e6]],    nil --[[e7]],
    nil --[[e8]],   nil --[[e9]],      nil --[[ea]],    nil --[[eb]],
    nil --[[ec]],   nil --[[ed]],      nil --[[ee]],    nil --[[ef]],
    nil --[[f0]],   nil --[[f1]],      nil --[[f2]],    nil --[[f3]],
    nil --[[f4]],   nil --[[f5]],      nil --[[f6]],    nil --[[f7]],
    nil --[[f8]],   nil --[[f9]],      nil --[[fa]],    nil --[[fb]],
    nil --[[fc]],   nil --[[fd]],      nil --[[fe]],    nil --[[ff]],
}

local sizes = { [0] =
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
      2,   3,   2,   3,   3,   2,   2,   2,   2,   2,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   3,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   1,   1,   3,   3,   3,   3,   3,   3,   3,
      3,   3,   3,   3,   3,   3,   3,   3,   3,   2, nil, nil,   1,   1,   1,   1,
      1,   1,   3,   3,   3,   3,   3,   3,   3,   5, nil,   3,   2,   3,   1,   1,
      3,   3,   1,   1, nil,   4,   3,   3,   5,   5, nil, nil, nil, nil, nil, nil,
    nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
    nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
    nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
}

-- tableswitch
sizes[0xaa] = function(b)
    local pos = b:tell()
    local pad = (4 - (pos & 3)) & 3
    b:seek(pad, 'cur')

    b:read('sdb') -- default
    local low = b:read('sdb')
    local high = b:read('sdb')

    return 1 + pad + 4 + 4 + 4 + (high - low + 1) * 4
end

-- lookupswitch
sizes[0xab] = function(b)
    local pos = b:tell()
    local pad = (4 - (pos & 3)) & 3
    b:seek(pad, 'cur')

    b:read('sdb') -- default
    local npairs = b:read('sdb')

    return 1 + pad + 4 + 4 + npairs * 8
end

-- wide
sizes[0xc4] = function(b)
    local op = b:read('ub')
    return op == 0x84 and 6 or 4
end

local deltas = { [0] =
      0,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   1,   1,   1,   2,   2,
      1,   1,   1,   1,   2,   1,   2,   1,   2,   1,   1,   1,   1,   1,   2,   2,
      2,   2,   1,   1,   1,   1,   2,   2,   2,   2,   1,   1,   1,   1,  -1,   0,
     -1,   0,  -1,  -1,  -1,  -1,  -1,  -2,  -1,  -2,  -1,  -1,  -1,  -1,  -1,  -2,
     -2,  -2,  -2,  -1,  -1,  -1,  -1,  -2,  -2,  -2,  -2,  -1,  -1,  -1,  -1,  -3,
     -4,  -3,  -4,  -3,  -3,  -3,  -3,  -1,  -2,   1,   1,   1,   2,   2,   2,   0,
     -1,  -2,  -1,  -2,  -1,  -2,  -1,  -2,  -1,  -2,  -1,  -2,  -1,  -2,  -1,  -2,
     -1,  -2,  -1,  -2,   0,   0,   0,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -2,
     -1,  -2,  -1,  -2,   0,   1,   0,   1,  -1,  -1,   0,   0,   1,   1,  -1,   0,
     -1,   0,   0,   0,  -3,  -1,  -1,  -3,  -3,  -1,  -1,  -1,  -1,  -1,  -1,  -2,
     -2,  -2,  -2,  -2,  -2,  -2,  -2,   0,   1,   0,  -1,  -1,  -1,  -2,  -1,  -2,
     -1,   0, nil, nil, nil, nil, nil, nil, nil, nil, nil,   1,   0,   0,   0,   0,
      0,   0,  -1,  -1, nil, nil,  -1,  -1,   0,   1, nil, nil, nil, nil, nil, nil,
    nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
    nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
    nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil, nil,
}

-- getstatic
deltas[0xb2] = function(b, cpool)
    if cpool then
        local fieldref = cpool[b:read('uw')]
        local nameAndType = cpool[fieldref.nameAndTypeIndex]
        local jdesc = cpool[nameAndType.descriptorIndex]
        return descutil.countSlots(jdesc.bytes)
    else
        return nil
    end
end

-- putstatic
deltas[0xb3] = function(b, cpool)
    local delta = deltas[0xb2](b, cpool)
    return delta and (-delta)
end

-- getfield
deltas[0xb4] = function(b, cpool)
    local delta = deltas[0xb2](b, cpool)
    return delta and (delta - 1)
end

-- putfield
deltas[0xb5] = function(b, cpool)
    local delta = deltas[0xb3](b, cpool)
    return delta and (delta - 1)
end

-- invokevirtual
deltas[0xb6] = function(b, cpool)
    local delta = deltas[0xb8](b, cpool)
    return delta and (delta - 1)
end

-- invokespecial
deltas[0xb7] = function(b, cpool)
    return deltas[0xb6](b, cpool)
end

-- invokestatic
deltas[0xb8] = function(b, cpool)
    if cpool then
        local methodref = cpool[b:read('uw')]
        local nameAndType = cpool[methodref.nameAndTypeIndex]
        local jdesc = cpool[nameAndType.descriptorIndex]

        local delta = -descutil.countSlots(jdesc.bytes)

        if not descutil.isVoid(jdesc.bytes) then
            delta = delta + 1
        end

        return delta
    else
        return nil
    end
end

-- invokeinterface
deltas[0xb9] = function(b, cpool)
    return deltas[0xb6](b, cpool)
end

-- wide
deltas[0xc4] = function(b, cpool)
    return deltas[b:read('ub')]
end

-- multianewarray
deltas[0xc5] = function(b, cpool)
    b:seek(2, 'cur')
    return b:read('ub')
end

return {
    opcodes = function(b, cpool)
        local iterator = function(s, var)
            local b = s[1]
            local pc = s[2]

            if pc >= b:size() then
                return nil
            end

            local cpool = s[3]

            b:seek(pc, 'set')
            local op = b:read('ub')
            
            local name = names[op]
            local size = type(sizes[op]) == 'number' and sizes[op] or sizes[op](b)
            local delta = type(deltas[op]) == 'number' and deltas[op] or deltas[op](b, cpool)

            s[2] = pc + size
            return pc, op, name, size, delta
        end

        return iterator, {b, 0, cpool}, 0
    end
}
