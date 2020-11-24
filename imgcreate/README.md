# imgcreate

**imgcreate** creates PNG, BMP, TGA, and JPEG images from pixels. It returns the images as strings, so it's the callers responsibility's to write it to disk. Pixels are sampled via a callback function.

## Bulding

It's just one file, either add it to your project or build a loadable Lua module with:

```
$ gcc -std=c99 -O2 -Werror -Wall -Wpedantic -Wno-unused-function -shared -fPIC -o imgcreate.so imgcreate.c
```

## Usage

### `imgcreate.png`

`imgcreate.png` creates a PNG image. Its arguments are:

1. Width of the image
1. Height of the image
1. Number of components
  * 1 for images with only the Y (brightness) component
  * 2 for YA images (Y + alpha)
  * 3 for RGB images (red, green, and blue)
  * 4 for RGBA images (RGB + alpha)
1. The callback function used to sample pixels
  * This function will be called for every pixel of the image, and must return 1 to 4 values depending on the number of components passed in the previous argument

Example:

```lua
local imgcreate = require 'imgcreate'

local png = imgcreate.png(256, 256, 3, function(x, y)
    return x, 0, y
end)

io.open('test.png', 'wb'):write(png):close()
```

### `imgcreate.bmp`, `imgcreate.tga` and `imgcreate.jpeg`

These functions work exactly the same way as `imgcreate.png`, but generate their respective formats. `imgcreate.jpeg` has an additional parameter before the callback function that specifies the desired quality, between 1 and 100.

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.
