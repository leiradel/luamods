# z80

Bindings for Andre Weissflog's [Z80 CPU emulator](https://github.com/floooh/chips/blob/master/chips/z80.h). The bindings are as close to the original implementation as possible.

## Building

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

The CPU object has methods to control the CPU, and to query and set the values of all Z80 registers plus some additional internal state. All functions that take a `z80_t*` argument have corresponding methods in the CPU object.

In particular, the `trap_cb` method accepts a function which will receive the CPU object, the `PC` of the **next** instruction, the ticks, and the CPU pins. This function will be called after the execution of every instruction. The function must return zero to signal that execution should continue, any other value will make the execution loop exit, with the value available via a call to the `trap_id` method. Call `trap_cb` with `nil` to disable the trap callback.

### Macros

All the macros that change the CPU pins have been converted to functions in the module. Since it's not possible to change a value in place in Lua, the functions take the current pins value and return a new one.

The other macros that have pin bits and masks are also available in the module.

### `z80.dasm`

`z80.dasm` returns the disassembling of an instruction and some additional information. It's signature is:

```lua
z80.dasm(
    program_counter, -- Program counter for the instruction being disassembled.
                     -- This argument is optional and defaults to 0.

    input_cb         -- A function that will be called repeatdly to provide the
                     -- opcodes for the instruction. If input_cb is a string or
                     -- a table, the opcodes will be read directly from it,
                     -- starting at start_index.

    start_index      -- The starting index to read bytes from strings and
                     -- tables, optional and defaults to 1.
)
```

`z80.dasm` returns:

1. A string with the disassembled instruction
1. The PC of the following instruction
1. The affected flags as an object (see below)
1. The number of cycles the instruction takes to execute
1. If the instruction takes a different number of cycles to execute under certain circumstances (i.e. a conditional jump instruction), a second number of cycles is returned. The exact meaning of the two values depend on the actual instruction.

The flags object has the following methods:

* `changed`: returns `true` if the flag is changed by the instruction, and `false` otherwise
* `unchanged`: returns `true` if the flag is unchanged by the instruction, and `false` otherwise
* `set`: returns `true` if the flag is set by the instruction, and `false` otherwise
* `reset`: returns `true` if the flag is reset by the instruction, and `false` otherwise

These methods accept a flag name as a string:

* `'S'` or `'s'` for the signal flag
* `'Z'` or `'z'` for the zero flag
* `'Y'`, `'y'`, or `'5'` for the undocumented `Y` flag
* `'H'` or `'h'` for the half carry flag
* `'X'`, `'x'`, or `'3'` for the undocumented `X` flag
* `'P'` or `'p'` for the parity/overflow flag
* `'N'` or `'n'` for the addition/subtraction flag
* `'C'` or `'c'` for the carry flag

The flags object also has a `__tostring` metamethod that returns a string representation of the object:

* The string with each character representing a flag, in the order `SZ5H3PNV`
* Changed flags appear as `'*'`
* Unchanged flags appear as `'-'`
* Set and reset flags appear as `'1'` and `'0'`, respectively

Example:

```lua
-- Print the following line three times:
-- BIT 1,(IX+3)    4       ***1**0-        20      nil
local mem = {0xdd, 0xcb, 0x03, 0x4e}

print(z80.dasm(function()
    local b = mem[1]
    table.remove(mem, 1)
    return b
end))

print(z80.dasm({0x3c, 0xdd, 0xcb, 0x03, 0x4e}, 2))
print(z80.dasm('\xed\xb0\xdd\xcb\x03\x4e', 3))
```

## Changelog

* 1.1.0
  * Added the `z80.dasm` function
* 1.0.0
  * First public release

## License

MIT, enjoy.
