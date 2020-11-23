# changeme

**changeme** is a little Lua module to make it easier to change fields in tables over time, using different easing functions.

## Bulding

It's just one file, either add it to your project or build a loadable Lua module with:

```
$ gcc -O2 -shared -fPIC -o changeme.so changeme.c -llua
```

The following macros can be defined to configure the module:

* `CHANGEME_FIELD_NAMES_SIZE` (default 24): the maximum number of characters the field names can occupy in a change (each field name occupy it's name length plus 1 NUL character; there's an additional NUL character at the end of all field names)
* `CHANGEME_MAX_FIELDS` (default 4): the maximum number of fields that a change can, hm, change
* `CHANGEME_INITIAL_ARRAY_SIZE` (default 32): the number of changes that will be reserved when the first change is created

While the limits above feel not very Luaish, they exist to keep the module simple and fast. If you do really need to change 16 fields simultaneously, either change the above limits or create as many changes as necessary to accommodate all of them given the configured limits.

## Usage

### `changeme.to`

`changeme.to` creates a change that will modify fields to an absolute value. Changes are only freed when they finish, so it's not necessary to keep them in variables to prevent them from dying a premature death.

```lua
changeme.to(
    table,                    -- The table that will have its fields changed.

    field_name, target_value, -- Field name as a string and its target value,
                              -- this pair can be repeated to change two or
                              -- more fields in the same change.

    duration,                 -- Duration of the change, can be in any unit as
                              -- long as it is used consistently.

    flags,                    -- The flags for the change (see below), this
                              -- field is optional and defaults to zero.

    callback                  -- A function that will be called when the change
                              -- ends, it will be called with the table as its
                              -- only parameter. The function is called even if
                              -- the change is configured to repeat.
)
```

After the duration it's possible to specify the following flags:

* `PAUSED`: the change will only begin after an explicitly call to `:start()` method
* `REPEAT`: the change will repeat when it finishes
* `AFTER`: the change will only update the fields at the end of the duration period

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

Example:

```lua
local changeme = require 'changeme'
local t = {x = 0, y = 100}

-- Change t.x from 0 (set above) to 100 and t.y from 100
-- (set above) to 0, linearly in 5 seconds. It's not necessary
-- to keep a reference to the change, it'll persist until
-- it finishes.
changeme.to(t, 'x', 100, 'y', 0, 5)

while t.x < 100 do
    changeme.update(0.1) -- 1/10 of a second has passed
    print(1, t.x, t.y)
end

-- Change t.x back to 0 using a cubic interpolation. The
-- change will start paused.
local x = changeme.to(t, 'x', 0, 5, changeme.CUBIC_IN + changeme.PAUSED)

for i = 1, 5 do
    changeme.update(0.1) -- the fields will not be updated
    print(2, t.x, t.y, x:status())
end

x:start() -- start the change

while t.x > 0 do
    changeme.update(0.1)
    print(3, t.x, t.y, x:status())
end

-- Change t.y to 100 abruptly after 2 seconds, and call a
-- function when it finishes.
changeme.to(t, 'y', 100, 2, changeme.AFTER, function(t)
    print('now t.y is 100')
end)

while t.y ~= 100 do
    changeme.update(0.1)
    print(4, t.x, t.y)
end

local t = {angle = 0}

-- Will keep t.angle spinning forever.
changeme.by(t, 'angle', 2 * math.pi, 2, changeme.REPEAT)

for i = 1, 100 do
    changeme.update(0.1) -- the fields will not be updated
    print(5, t.angle)
end
```

### `changeme.by`

`changeme.by` creates a change that will modify fields to a value relative to the current fields values. The parameters are the same as in `changeme.to`

```lua
local changeme = require 'changeme'
local t = {angle = 0}

-- Will keep t.angle spinning forever.
changeme.by(t, 'angle', 2 * math.pi, 2, changeme.REPEAT)
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
