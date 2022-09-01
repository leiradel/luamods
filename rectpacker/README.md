# rectpacker

**rectpacker** is a Lua binding for [Sean Barrett](https://github.com/nothings)'s [2D rectangle packer](https://github.com/nothings/stb/blob/master/stb_rect_pack.h).

## Building

It's just one file, either add it to your project or build a loadable Lua module with:

```
$ gcc -std=c99 -I. -O2 -Werror -Wall -Wpedantic -shared -fPIC -o rectpacker.so rectpacker.c
```

> Notice that it has to be able to find the include file `stb_rect_pack.h`.

## Usage

### `rectpacker.newPacker()`

`rectpacker.newPacker()` creates a new rectangle packer:

```lua
rectpacker.newPacker(
    width,  -- The width of the area where rectangles will be allocated from.

    height, -- The height of the area.

    nodes   -- Number of nodes for the temporary storage, defaults to the same
            -- value as the width parameter.
)
```

#### `packer:pack()`

Allocates an area in the rectangle configured in the packer. It returns the following values:

1. `true` if the allocation succeeded, in which case it also returns two more results:
    * The **x** coordinate of the top-left corner of the allocated rectangle.
    * The **y** coordinate of the top-left corner of the allocated rectangle.
1. `false` if the allocation failed, in which case no other values are returned.

> The interpretation of the coordinates is at the user's discretion, it could be the bottom-left corner or something else.

```lua
packer:pack(
    width, -- Width of the rectangle to allocate.

    height -- Height of the rectangle
)
```

When passed an array of rectangles (see below), `:pack()` will only return `true`, if it was able to pack all the rectangles in the array, or `false` if it wasn't.

```lua
packer:pack(
    rectangles -- An array of rectangles.
)
```

#### `packer:reset()`

Resets the packer so that the entire area is available for allocations again.

### `rectpacker.newRects()`

The packer is more efficient if called with many rectangles at once. To help with that, the module provides a dynamic array of rectangles that can be all passed at once to `packer:pack()`.

Rectangles can only be added at the end of the array. In addition to the width and height, each rectangle must provide an integer identifier which must be unique to each rectangle in the array. This is because the packer will reorder the rectangles in the array while packing them, so it's necessary to know which is which when the packer returns.

The array of rectangles is nice to enable efficient packing of atlases built in runtime, where new images can keep being added to the atlas until it's full.

If an initial capacity is not provided, the array will be allocated with 16 elements when the first rectangle is inserted.

```lua
rectpacker.newRects(
    initial_capacity -- The initial capacity of the array. The array will be
                     -- able to accomodate that number of rectangles before
                     -- dynamically resizing the elements to create space for
                     -- additional rectangles. Optional.
)
```

#### `rects:append()`

This method will append a rectangle at the end of the array.

```lua
rects:append(
    width,  -- The rectangle width.
    height, -- The rectangle height.
    id      -- An integer that uniquely identifies the rectangle in the array.
)
```

#### `rects:get()`

Returns information about the rectangle at the given 1-based index. It returns the the following six values:

1. `true` if that particular rectangle was packed, or `false` if it wasn't
1. The **x** coordinate of the top-left corner of the rectangle, or `nil` if the rectangle wasn't packed.
1. The **y** coordinate of the top-left corner of the rectangle, or `nil` if the rectangle wasn't packed.
1. The **width** of the rectangle.
1. The **height** of the rectangle.
1. The **id** of the rectangle.

```lua
rects:get(
    index -- 1-based index into the array
)
```

#### `rects:reset()`

Clears the array so that there are no rectangles in it. This doesn't shrink the allocated memory, it only resets the rectangle count to 0.

#### `#rects`

Returns the number of rectangles in the array.

## Example

```lua
local rectpacker = require 'rectpacker'
local packer = rectpacker.newPacker(256, 256)

print(packer:pack(128, 128)) -- prints true 0 0
print(packer:pack(128, 128)) -- prints true 128 0
print(packer:pack(192, 128)) -- prints true 0 128
print(packer:pack( 64,  64)) -- prints true 192 128
print(packer:pack( 64,  64)) -- prints true 192 192
print(packer:pack(  1,   1)) -- prints false

packer:reset()
print(packer:pack(1, 1)) -- prints true 0 0

local rects = rectpacker.newRects()

rects:append(128, 128, 1)
rects:append(128, 128, 2)
rects:append(192, 128, 3)
rects:append( 64,  64, 4)
rects:append( 64,  64, 5)

print(#rects, rects) -- prints 5	Rects(5/16)

--[[
The loop below will print the following:

1	false	nil	nil	128	128	1
2	false	nil	nil	128	128	2
3	false	nil	nil	192	128	3
4	false	nil	nil	64	64	4
5	false	nil	nil	64	64	5
]]
for i = 1, #rects do
    print(i, rects:get(i)) -- prints i false 0 0 width height id
end

packer:reset()

-- Prints false, which is interesting. The packer should be able to pack all
-- the five rectangles in the array but isn't, leaving the two 64x64 ones out.
-- Certainly it's due to our contrived example, where the rectangles make a
-- perfect fit in the available area when allocated one after the other.
print(packer:pack(rects))

--[[
The loop below will print the following:

1	true	0	128	128	128	1
2	true	128	128	128	128	2
3	true	0	0	192	128	3
4	false	nil	nil	64	64	4
5	false	nil	nil	64	64	5

Notice the invalid coordinates for the rectangles that weren't packed.
]]
for i = 1, #rects do
    print(i, rects:get(i))
end

rects:reset()
print(#rects, rects) -- prints 0	Rects(0/16)
```

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.
