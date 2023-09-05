# fsmc

`fsmc` is a `ddlt` example that generates C code for a [Finite State Machine](https://en.wikipedia.org/wiki/Finite-state_machine) described in a file.

## EBNF

```ebnf
start = [header], [implementation], fsm ;

header = 'header', freeform ;

implementation = 'implementation', freeform ;

fsm = 'fsm', id, parameters, '{',
      [fields], [before], [after], state, {state},
      '}' ;

(*
Parameters are one identifier for the type and another for the variable name.
If a complex type is necessary, use the code block to create a typedef for it,
and use the typedef here.
*)
parameters = '(', [id, id, {',', id, id}], ')' ;

(*
Fields is a free form block that will be added to the compiled FSM structure.
*)
fields = 'fields', freeform ;

(*
The before block contains code that will be executed before a transition
occurs. The transition can be canceled (leaving the FSM in its old state) by
using the 'forbid' keyword in the code block, or allowed early by using the
'allow' keyword.
*)
before = 'before', freeform ;

(*
The after block contains code that will be executed after a transition occurs.
*)
after = 'after', freeform ;

(*
States are defined by a set of transitions to other states. They can also have
a before block that will be executed after the FSM before block, and an after
block that will be executed before the FSM after block.

The special 'stack' state describes transitions that push the current state
before the transition, and pop it back when the opposite transition occurs.

The states involved in transitions in the 'stack' state must be declared like
all other states, but they cannot have transitions of their own. However, they
are allowed to have 'before' and 'after' blocks.

Transitions declared in the 'stack' state can have a list of allowed states
from which the transition is allowed. If the list is absent, the transition
can be applied to all states.
*)
state = (id | 'stack'), '{', [before], [after], {transition}, '}' ;

transition = [id, {',' id} ':'],
             id, parameters, (direct | sequence | pop) ;

(*
A direct transition to another state.
*)
direct = '=>', id, (freeform | ';') ;

(*
A sequence of transitions that will arrive at a final state.
*)
sequence = '=>', id, arguments, {'=>', id, arguments} ';' ;

(*
A transition that pops a state from the stack.
*)
pop = freeform | ';' ;

arguments = '(', [id, {',', id}], ')' ;

(*
A free form block can have any characters, including closing braces. Braces are
balanced, so "{ if (x) { ... } }" is a valid free form block.
*)
freeform = '{', ? all characters ?, '}' ;

id = ? C identifier ? ;
```
