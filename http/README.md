# http

**http** is a module that performs non-blocking HTTP requests, passing returned data to a callback function.

## Building

It's just one file, either add it to your project or build a loadable Lua module with:

```
$ gcc -std=c99 -O2 -Werror -Wall -Wpedantic -shared -fPIC -o http.so http.c
```

> On Linux, **http** requires `lincurl` to build.

## Usage

### `http.get()`

`rectpacker.get()` initiates a HTTP GET:

```lua
http.get(
    utl,     -- The URL to retrieve the contents from.

    callback -- The callback that receives results.
)
```

The callback will receive one or two arguments depending on the outcome of the operation. The first argument is always a string with the type of the result:

* `'header'`: A header line has been received, the second argument is the header line. There can be multiple calls of this type.
* `'data'`: More data has arrived from the server, the second argument is a string containing the data. There can be multiple calls of this type.
* `'end'`: The operation has finished, the second argument is `nil`.
* `'error'`: There was an error performing the HTTP operation, the second argument has the error message.

### `http.tick()`

This function must be called on a regular basis to process incoming data and the calling of the callback functions. It doesn't require any parameter.

## Example

```lua
local http = require 'http'

local done, body = false, {}

http.get('https://google.com', function(what, data)
    if what == 'header' then
        print(data)
    elseif what == 'data' then
        body[#body + 1] = data
    end

    done = what == 'end'
end)

while not done do
    http.tick()
end

print(table.concat(body))
```

The code above can result is something like the following:

```
HTTP/2 301 
location: https://www.google.com/
content-type: text/html; charset=UTF-8
date: Mon, 05 Sep 2022 22:17:11 GMT
expires: Mon, 05 Sep 2022 22:17:11 GMT
cache-control: private, max-age=2592000
server: gws
content-length: 220
x-xss-protection: 0
x-frame-options: SAMEORIGIN
set-cookie: CONSENT=PENDING+416; expires=Wed, 04-Sep-2024 22:17:11 GMT; path=/; domain=.google.com; Secure
p3p: CP="This is not a P3P policy! See g.co/p3phelp for more info."
alt-svc: h3=":443"; ma=2592000,h3-29=":443"; ma=2592000,h3-Q050=":443"; ma=2592000,h3-Q046=":443"; ma=2592000,h3-Q043=":443"; ma=2592000,quic=":443"; ma=2592000; v="46,43"
<HTML><HEAD><meta http-equiv="content-type" content="text/html;charset=utf-8">
<TITLE>301 Moved</TITLE></HEAD><BODY>
<H1>301 Moved</H1>
The document has moved
<A HREF="https://www.google.com/">here</A>.
</BODY></HTML>
```

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.
