# z80

Bindings for Andre Weissflog's [Z80 CPU emulator](https://github.com/floooh/chips/blob/master/chips/z80.h). The bindings are as close to the original implementation as possible.

## Bulding

It's just one file, either add it to your project or build a loadable Lua module with:

```
$ gcc -O2 -shared -fPIC -o z80.so z80.c -llua
```

## Usage

### `z80.init`

`z80.init` creates a new Z80 CPU emulator. The only parameter is the tick function, which will receive the CPU object, the number of cycles for the current invocation, and the CPU pins. The tick function must return the (possibly) updated pins.

Example:

```lua
local z80 = require 'z80'

local cpu = z80.init(function(cpu, num_ticks, pins)
    if (pins & z80.MREQ) ~= 0 then
        local address = z80.GET_ADDR(pins)

        if (pins & z80.RD) ~= 0 then
            pins = z80.SET_DATA(pins, memory[address])
        elseif (pins & z80.WR) ~= 0 then
            memory[address] = z80.GET_DATA(pins)
        end
    end

    return pins
end)

cpu:exec(1)
```

### Methods

The CPU object has methods to control the CPU, and to query and set the values of all Z80 registers plus some additional internal state. All functions that take a `z80_t*` argument have corresponding methos in the CPU object.

In particular, the `trap_cb` method accepts a function which will receive the CPU object, the `PC` of the **next** instruction, the ticks, and the CPU pins. This function will be called after the execution of every instruction. The function must return zero to signal that execution should continue, any other value will make the execution loop exit, with the value available via a call to the `trap_id` method. Call `trap_cb` with `nil` to disable the trap callback.

### Macros

All the macros that change the CPU pins have been converted to functions in the module. Since it's not possible to change a value in place in Lua, the functions take the current pins value and return a new one.

The other macros that have pin bits and masks are also available in the module.

## Remarks

**z80** will use the **access** module if available, to turn the module into a read-only object and make it impossible to change fields in it. This slows down things a bit since everything is accessed via a proxy object. If **access** is not available, the returned module will be a regular Lua table.

## Changelog

* 1.0.0
  * First public release

## License

MIT, enjoy.
