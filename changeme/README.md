# changeme

**changeme** is a little Lua module to make it easy to interpolate values over time, using different easing functions.

## Building

It's just one file, either add it to your project or build a loadable Lua module with:

```
$ gcc -std=c99 -O2 -Werror -Wall -Wextra -Wpedantic -shared -fPIC -o changeme.so changeme.c
```

The following macros can be defined to configure the module:

* `CHANGEME_MAX_PAIRS` (default 4): The maximum number of `<start, target>` value pairs that one change can interpolate.
* `CHANGEME_INITIAL_ARRAY_SIZE` (default 32): the number of changes that will be reserved when the first change is created

While the limits above feel not very Lua-ish, they exist to keep the module simple and fast. If you do really need to change 16 values simultaneously, either change the above limits or create as many changes as necessary to accommodate all of them given the configured limits.

## Usage

### `changeme.to`

`changeme.to` creates a change that interpolate values to a target absolute value.

```lua
changeme.to(
    duration,                    -- Duration of the change, can be in any unit
                                 -- as long as it is used consistently.

    flags,                       -- The flags for the change and easing function
                                 -- to use (see below).

    initial_value, target_value, -- 0 or more pairs of initial and target values
                                 -- that will be updated by the change, up to
                                 -- CHANGEME_MAX_PAIRS.

    callback                     -- A function that will be called when the
                                 -- change ends. It's' called with the change
                                 -- itself as the first parameter, and the
                                 -- current interpolated values of each of the
                                 -- value pairs.
)
```

After the duration it's possible to specify the following flags:

* `PAUSED`: the change will only begin after an explicitly call to `:start()` method
* `REPEAT`: the change will repeat when it finishes
* `FINISHED`: the change will only update the fields at the end of the duration period

The available easing functions are:

||||
|---|---|---|
|`LINEAR`|||
|`BACK_IN`|`BACK_OUT`|`BACK_IN_OUT`|
|`BOUNCE_IN`|`BOUNCE_OUT`|`BOUNCE_IN_OUT`|
|`CIRCULAR_IN`|`CIRCULAR_OUT`|`CIRCULAT_IN_OUT`|
|`CUBIC_IN`|`CUBIC_OUT`|`CUBIC_IN_OUT`|
|`ELASTIC_IN`|`ELASTIC_OUT`|`ELASTIC_IN_OUT`|
|`EXPONENTIAL_IN`|`EXPONENTIAL_OUT`|`EXPONENTIAL_IN_OUT`|
|`QUADRATIC_IN`|`QUADRATIC_OUT`|`QUADRATIC_IN_OUT`|
|`QUARTIC_IN`|`QUARTIC_OUT`|`QUARTIC_IN_OUT`|
|`QUINTIC_IN`|`QUINTIC_OUT`|`QUINTIC_IN_OUT`|
|`SINE_IN`|`SINE_OUT`|`SINE_IN_OUT`|

It's possible to see how these functions map the input to output values [here](https://easings.net/en).

A changes is freed when:

1. There are no more references to it in Lua **and** it is paused.
1. It has finished and wasn't set to repeat.
1. It is explicitly killed.

Running changes are never freed, even if there are no more references to it in Lua. When they finish, they'll be automatically freed. That means you don't have to keep a reference to a running change to keep it alive, and it also means that changes set to repeat will never be freed, unless they're explicitly killed.

Example:

```lua
local changeme = require 'changeme'
local x, y = 0, 100

-- Change x from 0 (set above) to 100 and y from 100 (set above) to 0, linearly
-- in 5 time units. It's not necessary to keep a reference to the change, it'll
-- persist until it finishes.
changeme.to(5, changeme.LINEAR, x, 100, y, 0, function(change, ix, iy)
    x, y = ix, iy
end)

while x < 100 do
    changeme.update(0.1) -- 1/10 of a time unit has passed
    print(1, x, y)
end

-- Change x back to 0 using a cubic interpolation. The change will start paused.
local c = changeme.to(5, changeme.CUBIC_IN + changeme.PAUSED, x, 0, function(change, ix)
    x = ix
end)

for i = 1, 5 do
    changeme.update(0.1) -- the fields will not be updated
    print(2, x, y, c:status())
end

c:start() -- start the change

while x > 0 do
    changeme.update(0.1)
    print(3, x, y, c:status())
end

-- Change y to 100 abruptly after 2 seconds.
changeme.to(2, changeme.AFTER, y, 100, function(change, iy)
    y = iy
    print('now y is 100')
end)

while y ~= 100 do
    changeme.update(0.1)
    print(4, x, y)
end
```

### `changeme.by`

`changeme.by` creates a change that will modify fields to a value relative to the current fields values. The parameters are the same as in `changeme.to`

```lua
local changeme = require 'changeme'
local angle = 0

-- Will keep angle spinning forever, completing 360 degrees each two seconds.
changeme.by(2, 2 * math.pi, changeme.REPEAT, function(change, iangle)
    angle = iangle
end)

for i = 1, 100 do
    changeme.update(0.1)
    print(5, angle)
end
```

### `changeme.update`

`changeme.update` must be called regularly with the elapsed time since the last call, using the same unit as used in `changeme.to` and `changeme.by`.

### `:start`

The `start` method will start a change that was created paused with the `PAUSED` flag.

### `:kill`

The `kill` method will kill a change. Useful when a change was created with `REPEAT` but is no longer necessary.

### `:status`

The `status` method will return a string with the status of the change. The names follow the ones from Lua coroutines:

* `"dead"`: when the change has finished
* `"suspended"`: when the changes was created in pause mode as has not been started
* `"running"`: when the change is active

## Changelog

* 2.0.0
  * Changes don't actuate on tables anymore, but interpolate explicit values
  * The callback function is mandatory, and receive the change itself followed by the interpolated values
* 1.2.2
  * Fixes to compile with C99
* 1.2.1
  * Moved module to a subfolder
  * Clarifications and fixed to the README
* 1.2.0
  * Adopted semantic versioning
  * Fixed a stupid bug where the free list would end up pointing to invalid addresses when `s_changes` grows
  * Added `_URL` to the module
* 1.1
  * Added module information: `_COPYRIGHT`, `_LICENSE`, `_VERSION`, `_NAME`, and `_DESCRIPTION`
  * Fixed field names not being terminated with two nulls
  * Make it possible to reuse changes even if there are still references to it in Lua land
* 1.0
  * First public release

## License

MIT, enjoy.
