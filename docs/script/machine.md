Script Virtual Machine
======================

Stack
-----

There are really two stacks:
- data stack for function arguments and local values
- call stack for return addresses and debug data

### Data Stack Frame

    (top)   +---------------+  (lower address)
            |   2nd local   |
            +---------------+
            |   1st local   |
    base    +===============+ 
            |   non-locals  |
    (args)  +===============+ 
            | 1st argument  |
            +---------------+
            | 2nd argument  |
    (ret)   +---------------+  (higher address)

Data stack grows down, to lower addresses.

Arguments are pushed in reversed order, which has two benefits:

- functions can consume arguments from top - pull first arg, pull second arg,
  push result
- tuple arguments are equivalent to unpacked fields - it doesn't matter
  if each argument is passed separately or all of them in tuple

Non-locals are pushed as a tuple, above the arguments. They are effectively
pushed in reversed order, the same way as the arguments.

Only the `base` address is actually saved in the Call Stack, the others are
computed by compiler and encoded directly into instructions.
For example, when compiling the function, compiler knows if it has
any non-locals, so it can compute `(args)` offset from `base` and use that
when referencing arguments. It can compute address of each argument in the
same way.

Everything below `base` is static - the addresses don't change
while the function runs. On the other hand, the locals area above `base`
is dynamic and the stack `(top)` can move freely.


### Call Stack Frame

Call stack is maintained solely by the VM. When a function reaches end of its
code, VM is responsible for popping the frame from call stack and restoring
program counter to caller site.

Frame contains:
- return address: function / instruction
- base pointer: parameters and non-locals are below, local values above

The base pointer is accessible to program via instructions which take offset
from current base as their parameter.


### Calling convention

Caller pushes args to stack, in reversed order (1st arg last).
Then it either calls a static function (CALL), or executes function object
(EXECUTE). If the function object contains closure, it is unpacked on stack
by VM before calling the function.

Static function:

    Caller code:              Stack when entering called function:
    
    PUSH <2nd arg>            base    +===============+
    PUSH <1st arg>                    | 1st argument  |
    CALL <static func>                +---------------+
                                      | 2nd argument  |
                                      +---------------+


Function object:

    Caller code:              Stack when entering called function:
    
    PUSH <2nd arg>            base    +===============+
    PUSH <1st arg>                    |  non-locals   |
    PUSH <function object>            +===============+
    EXECUTE                           | 1st argument  |
                                      +---------------+
                                      | 2nd argument  |
                                      +---------------+

The called function is responsible for consuming the arguments and non-locals.
Those have to be replaced by returned value before exiting.

    Callee code:              Stack when returning to caller:
      
    PUSH <ret val>            base    +===============+
    DROP <N args + nonloc>            .               .
    // RETURN                 top     +---------------+
                                      | return value  |
                                      +---------------+

Pseudo-functions like ADD, GREATER_THAN behave the same way: they expect
the operands in reversed order and replace them with result.


Heap
----

Some of the values live on heap, with only a pointer on stack. All heap values are
reference-counted. When all references are removed from stack, they are freed immediately.
As all values are immutable, we can still think about the values as they were directly
on stack. Actually, optimizing compiler can decide if the value goes to stack or not
and it may also decide to overwrite heap value with only single reference.

All heap values have a header with refcount:

    4B refcount
    xB data

The size of data is not part of the header, but may be the first item of the data
(this is the case for strings and arrays).
