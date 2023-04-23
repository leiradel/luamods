local function xlateAccessFlags(flags)
    local set = {}

    set.public    = (flags & 0x0001) ~= 0
    set.private   = (flags & 0x0002) ~= 0
    set.protected = (flags & 0x0004) ~= 0
    set.static    = (flags & 0x0008) ~= 0
    set.final     = (flags & 0x0010) ~= 0
    set.super     = (flags & 0x0020) ~= 0
    set.volatile  = (flags & 0x0040) ~= 0
    set.transient = (flags & 0x0080) ~= 0
    set.native    = (flags & 0x0100) ~= 0
    set.interface = (flags & 0x0200) ~= 0
    set.abstract  = (flags & 0x0400) ~= 0
    set.strict    = (flags & 0x0800) ~= 0

    return set
end

local function readConstantClass(b)
    return {
        tag = 'class',
        nameIndex = b:read('uwb')
    }
end

local function readConstantFieldref(b)
    return {
        tag = 'fieldref',
        classIndex = b:read('uwb'),
        nameAndTypeIndex = b:read('uwb')
    }
end

local function readConstantMethodref(b)
    return {
        tag = 'methodref',
        classIndex = b:read('uwb'),
        nameAndTypeIndex = b:read('uwb')
    }
end

local function readConstantInterfaceMethodref(b)
    return {
        tag = 'interfaceMethodref',
        classIndex = b:read('uwb'),
        nameAndTypeIndex = b:read('uwb')
    }
end

local function readConstantString(b)
    return {
        tag = 'string',
        stringIndex = b:read('uwb')
    }
end

local function readConstantInteger(b)
    return {
        tag = 'integer',
        -- bytes is the actual integer, not the bytes.
        bytes = b:read('sdb')
    }
end

local function readConstantFloat(b)
    return {
        tag = 'float',
        -- bytes is the actual float, not the bytes.
        bytes = b:read('fb')
    }
end

local function readConstantLong(b)
    return {
        tag = 'long',
        -- bytes is the actual long, not the bytes.
        bytes = b:read('sqb')
    }
end

local function readConstantDouble(b)
    return {
        tag = 'double',
        -- bytes is the actual double, not the bytes.
        bytes = b:read('db')
    }
end

local function readConstantNameAndType(b)
    return {
        tag = 'nameAndType',
        nameIndex = b:read('uwb'),
        descriptorIndex = b:read('uwb')
    }
end

local function readConstantUtf8(b)
    local length = b:read('uwb')
    local bytes = b:read(length)

    return {
        tag = 'utf8',
        length = length,
        -- bytes is the actual string, not the bytes.
        bytes = bytes
    }
end

local function readConstant(b)
    local tag = b:read('ub')

    if tag == 7 then
        return readConstantClass(b)
    elseif tag == 9 then
        return readConstantFieldref(b)
    elseif tag == 10 then
        return readConstantMethodref(b)
    elseif tag == 11 then
        return readConstantInterfaceMethodref(b)
    elseif tag == 8 then
        return readConstantString(b)
    elseif tag == 3 then
        return readConstantInteger(b)
    elseif tag == 4 then
        return readConstantFloat(b)
    elseif tag == 5 then
        return readConstantLong(b)
    elseif tag == 6 then
        return readConstantDouble(b)
    elseif tag == 12 then
        return readConstantNameAndType(b)
    elseif tag == 1 then
        return readConstantUtf8(b)
    else
        error(string.format('invalid tag in constant pool: %u', tag))
    end
end

local function readConstantPool(n, b)
    local cpool = {n = n}
    local i = 1

    while i <= n do
        local const = readConstant(b)
        cpool[i] = const
        i = i + 1

        if const.tag == 'long' or const.tag == 'double' then
            -- Longs and doubles take two slots.
            i = i + 1
        end
    end

    return cpool
end

local function readInterfaces(n, b)
    local interfaces = {n = n}

    for i = 1, n do
        interfaces[i] = b:read('uwb')
    end

    return interfaces
end

local readAttributes

local function readConstantValue(b)
    return {
        tag = 'constantValue',
        index = b:read('uwb')
    }
end

local function readExceptionTable(n, b)
    local table = {n = n}

    for i = 1, n do
        table[i] = {
            startPc = b:read('uwb'),
            endPc = b:read('uwb'),
            handlerPc = b:read('uwb'),
            catchType = b:read('uwb')
        }
    end

    return table
end

local function readCode(b, cpool)
    local maxStack = b:read('uwb')
    local maxLocals = b:read('uwb')
    local codeLength = b:read('udb')

    local pos = b:tell()
    local bytecode = b:sub(pos, codeLength)
    b:seek(codeLength, 'cur')

    local exceptionTableLength = b:read('uwb')
    local exceptionTable = readExceptionTable(exceptionTableLength, b)
    local attributesCount = b:read('uwb')
    local attributes = readAttributes(attributesCount, b, cpool)

    return {
        tag = 'code',
        maxStack = maxStack,
        maxLocals = maxLocals,
        bytecode = bytecode,
        exceptionTable = exceptionTable,
        attributes = attributes
    }
end

local function readExceptions(b)
    local n = b:read('uwb')
    local indexTable = {n = n}

    for i = 1, n do
        indexTable[i] = b:read('uwb')
    end

    return {
        tag = 'exceptions',
        indexTable = indexTable
    }
end

local function readSynthetic(b)
    return {
        tag = 'synthetic'
    }
end

local function readSourceFile(b)
    return {
        tag = 'sourceFile',
        index = b:read('uwb')
    }
end

local function readLineNumberTable(b)
    local n = b:read('uwb')
    local table = {n = n}

    for i = 1, n do
        table[i] = {
            startPc = b:read('uwb'),
            lineNumber = b:read('uwb')
        }
    end

    return {
        tag = 'lineNumberTable',
        table = table
    }
end

local function readLocalVariableTable(b)
    local n = b:read('uwb')
    local table = {n = n}

    for i = 1, n do
        table[i] = {
            startPc = b:read('uwb'),
            length = b:read('uwb'),
            nameIndex = b:read('uwb'),
            descriptorIndex = b:read('uwb'),
            index = b:read('uwb')
        }
    end

    return {
        tag = 'localVariableTable',
        table = table
    }
end

local function readDeprecated(b)
    return {
        tag = 'deprecated'
    }
end

local function readInnerClasses(b)
    local n = b:read('uwb')
    local classes = {n = n}

    for i = 1, n do
        classes[i] = {
            innerClassInfoIndex = b:read('uwb'),
            outerClassInfoIndex = b:read('uwb'),
            innerNameIndex = b:read('uwb'),
            innerClassAccessFlags = xlateAccessFlags(b:read('uwb'))
        }
    end

    return {
        tag = 'innerClasses',
        classes = classes
    }
end

local function readAttribute(b, cpool)
    local nameIndex = b:read('uwb')
    local name = cpool[nameIndex]

    if type(name) ~= 'table' or name.tag ~= 'utf8' then
        error(string.format('invalid attribute nameIndex: %u', nameIndex))
    end

    local length = b:read('udb')
    local tag = name.bytes

    if tag == 'ConstantValue' then
        return readConstantValue(b)
    elseif tag == 'Code' then
        return readCode(b, cpool)
    elseif tag == 'Exceptions' then
        return readExceptions(b)
    elseif tag == 'Synthetic' then
        return readSynthetic(b)
    elseif tag == 'SourceFile' then
        return readSourceFile(b)
    elseif tag == 'LineNumberTable' then
        return readLineNumberTable(b)
    elseif tag == 'LocalVariableTable' then
        return readLocalVariableTable(b)
    elseif tag == 'Deprecated' then
        return readDeprecated(b)
    elseif tag == 'InnerClasses' then
        return readInnerClasses(b)
    else
        -- Skip unknown attribute
        b:seek(length, 'cur')
    end
end

readAttributes = function(n, b, cpool)
    local attributes = {n = n}

    for i = 1, n do
        local attribute = readAttribute(b, cpool)

        if attribute then
            attributes[i] = attribute
            attributes[attribute.tag] = attribute
        end
    end

    return attributes
end

local function readFields(n, b, cpool)
    local fields = {n = n}

    for i = 1, n do
        local accessFlags = xlateAccessFlags(b:read('uwb'))
        local nameIndex = b:read('uwb')
        local descriptorIndex = b:read('uwb')
        local attributesCount = b:read('uwb')
        local attributes = readAttributes(attributesCount, b, cpool)

        local field = {
            accessFlags = accessFlags,
            nameIndex = nameIndex,
            descriptorIndex = descriptorIndex,
            attributes = attributes
        }

        local name = cpool[nameIndex].bytes
        local descriptor = cpool[descriptorIndex].bytes

        fields[i] = field
        fields[string.format('%s(%s)', name, descriptor)] = field
    end

    return fields
end

local function readMethods(n, b, cpool)
    local methods = {n = n}

    for i = 1, n do
        local accessFlags = xlateAccessFlags(b:read('uwb'))
        local nameIndex = b:read('uwb')
        local descriptorIndex = b:read('uwb')
        local attributesCount = b:read('uwb')
        local attributes = readAttributes(attributesCount, b, cpool)

        local method = {
            accessFlags = accessFlags,
            nameIndex = nameIndex,
            descriptorIndex = descriptorIndex,
            attributes = attributes
        }

        local name = cpool[nameIndex].bytes
        local descriptor = cpool[descriptorIndex].bytes

        methods[i] = method
        methods[string.format('%s%s', name, descriptor)] = method
    end

    return methods
end

return function(b)
    local magic = b:read('udb')

    if magic ~= 0xcafebabe then
        error(string.format('invalid magic number: 0x%08x', magic))
    end

    local minor = b:read('uwb')
    local major = b:read('uwb')

    if major > 49 then
        -- Versions greater than JDK 1.5 are an error.
        error(string.format('unsupported class version %u.%u', major, minor))
    end

    local cpoolCount = b:read('uwb') - 1
    local cpool = readConstantPool(cpoolCount, b)

    local accessFlags = xlateAccessFlags(b:read('uwb'))
    local thisClass = b:read('uwb')
    local superClass = b:read('uwb')
    
    local interfacesCount = b:read('uwb')
    local interfaces = readInterfaces(interfacesCount, b)

    local fieldsCount = b:read('uwb')
    local fields = readFields(fieldsCount, b, cpool)

    local methodsCount = b:read('uwb')
    local methods = readMethods(methodsCount, b, cpool)

    local attributesCount = b:read('uwb')
    local attributes = readAttributes(attributesCount, b, cpool)

    return {
        magic = magic,
        major = major,
        minor = minor,
        constantPool = cpool,
        accessFlags = accessFlags,
        thisClass = thisClass,
        superClass = superClass,
        interfaces = interfaces,
        fields = fields,
        methods = methods,
        attributes = attributes
    }
end
