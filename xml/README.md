# xml

**xml** is a simple XML parser implemented in Lua. It's the exact same code as found in the [classic Lua-only](http://lua-users.org/wiki/LuaXml) implementation by Roberto Ierusalimschy tweaked to allow for XML namespaces, and with additional changes to implement the semantics described by Yutaka Ueno since the link to his implementation is broken.

## Building

**xml** is a pure Lua module.

## Usage

### `xml.parse`

Example:

```lua
local xml = require 'xml'

local t = xml.parse[[
<methodCall kind="xuxu">
    <methodName>examples.getStateName</methodName>
    <params>
        <param>
            <value><i4>41</i4></value>
        </param>
    </params>
</methodCall>
]]

print(t.xml) -- Prints "methodCall"
print(t.kind) -- Prints "xuxu"

for i = 1, #t do
    print('', i, t[i].xml) -- Prints "1 methodName" and "2 params"
end
```

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.
