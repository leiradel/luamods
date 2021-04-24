local function iterate(file)
    local section = nil
    local count = 0

    local function iterator(file)
        while true do
            local line = file:read('l')
            count = count + 1

            if not line then
                return
            end

            local comment = line:find(';', 1, true)

            if comment then
                line = line:sub(1, comment - 1)
            end

            local key, value = line:match('^([^%s]+)%s*=%s*(.-)\r?$')

            if key and value then
                if not section then
                    error('property without a previous section header in line ' .. count)
                end

                return section, key, value, count
            else
                local name = line:match('^%[([^%]]*)%]%s-$')

                if name then
                    if #name == 0 then
                        error('empty section name in line ' .. count)
                    end

                    section = name
                elseif not line:match('^(%s*)$') then
                    error('syntax error in line ' .. count)
                end
            end
        end
    end

    return iterator, file
end

local function load(path)
    local function doload(path)
        local file = assert(io.open(path, 'r'))
        local ini = {}

        for section, key, value in iterate(file) do
            local data = ini[section] or {}
            ini[section] = data
            data[key] = value
        end

        return ini
    end

    local ok, ini = pcall(doload, path)

    if ok then
        return ini
    else
        return nil, ini
    end
end

return {
    iterate = iterate,
    load = load
}
