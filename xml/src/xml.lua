local function parseargs(s, e)
    string.gsub(s, "([%-%w]+)=([\"'])(.-)%2", function (w, _, a)
      e[w] = a
    end)
end
      
local function collect(s)
    local stack = {}
    local top = {}
    table.insert(stack, top)
    local ni,c,label,xarg, empty
    local i, j = 1, 1
    while true do
        ni,j,c,label,xarg, empty = string.find(s, "<(%/?)([%w:]+)(.-)(%/?)>", i)
        if not ni then break end
        local text = string.sub(s, i, ni-1)
        if not string.find(text, "^%s*$") then
            table.insert(top, text)
        end
        if empty == "/" then  -- empty element tag
            local e = {xml=label}
            parseargs(xarg, e)
            table.insert(top, e)
        elseif c == "" then   -- start tag
            top = {xml=label}
            parseargs(xarg, top)
            table.insert(stack, top)   -- new level
        else  -- end tag
            local toclose = table.remove(stack)  -- remove top
            top = stack[#stack]
            if #stack < 1 then
                error("nothing to close with "..label)
            end
            if toclose.xml ~= label then
                error("trying to close "..toclose.xml.." with "..label)
            end
            table.insert(top, toclose)
        end
        i = j+1
    end
    local text = string.sub(s, i)
    if not string.find(text, "^%s*$") then
        table.insert(stack[#stack], text)
    end
    if #stack > 1 then
        error("unclosed "..stack[#stack].xml)
    end
    return stack[1]
end

return {
    parse = function(xml)
        local root = collect(xml)
        return root[1]
    end
}
