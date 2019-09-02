Lambda Script v0.2
==================
***Syntax reference***

[TOC]

Lambda Script is a functional language, strongly and statically typed,
with function overloading and generics (type variables).

The language is designed to allow small and easily embeddable implementation,
with possibility of compilation to native code.

This document presents the syntax and semantics by example.

An interpreter is being developed in C++ together with this description.
The interpreter consists of a C++ static library (`libxci-script`)
and a REPL interpreter (`demo_script`). The implementation is a little behind
this document and not everything described here has to be implemented.
This is design document, with the ideas. The design may change during implementation.

The implementation should completely match the specification when it reaches
version 1.0.

See `xci::script` in [xcikit](https://github.com/rbrich/xcikit) codebase.


Syntax Elements
---------------

C++ style comments:

    // inline comment
    /* possibly multiline comment */

Scoped blocks:

    // define some names in a scope:
    { a = 1; b = 2 }    // the whole expression is evaluated to `none`  
    a                   // ERROR - `a` is not defined in outer scope
    c = { a = 1; a }    // block returns `a`, `c` evaluates to `1`

- Semicolons are separators, not required after last expression and before EOL/EOF
- The block has a return value which is the result of last expression.
- Definitions don't return the value - explicit expression is required instead.

Function call:

    add 1 2
    sub (1 + 2) 3   // => 0
    1 .add 2        // infix style

- Function call syntax is minimalistic - there are no commas or parens.
- Parens can be used around each single argument.
- Infix operators and other function calls require parens around them.

About infix function calls:
- left-hand side expression is passed as first argument, right-hand side
  expressions are passed as following arguments (zero or more of them)
- spaces around the dot are optional, but numbers might need parenthesis:

      one = 1
      one.add 2
      1.add 2  // syntax error!
      (1).add 2  // ok
      one. add 2  // ok, but bad style
      1 . add 2   // ok, but bad style
      "{} {}" .format "hello" 91
      "string".len

- unlike infix operators, functions have no precedence - they are always
  evaluated from left to right:
  
      1 .add 2 .mul 3  // => 9
      (1 .add 2).mul 3  // => 9
      1 .add (2 .mul 3)  // => 7

Infix and prefix operators, operator precedence:

    1 + 2 / 3 == 1 + (2 / 3)
    -(1 + 2)

Definition (naming a value) can occur only once for each name in a scope:

    i = 1               // auto type
    j = 1 + 2
    i = j               // ERROR - redefinition of a name
    k = add2 1 2
    l:Int32 = k         // definition with type declaration
    s:String = "XCI"    // UTF-8 string

Define a function with parameters:

    add2 = |a b| {a + b}   // generic function - works with any type supported by op+
    add2 = |a:t b:t| -> t {a + b}  // same as above, but with explicit type variable
    add2 = |a:Int32 b:Int32|->Int32 {a + b}   // specific, with type declarations
    
    // function definition can span multiple lines
    add2 = | a:Int32 b:Int32 | -> Int32
    {
        a + b
    }
    
    // possible program main function
    main = | args:[String] | -> Void {
        print "Hello World!"
    }

Function call can explicitly name the arguments:

    make_book = | name:String author:String isbn:Int32 | -> MyBook
        { MyBook(name, author, isbn) }
    make_book name="Title" author="Karel IV" isbn=12345

This allows rearranging the arguments, but it doesn't allow skipping arguments
in middle (the last arguments might be left out to make partial call).

It also requires that the argument names are available together with function
prototype.

Pass a function as an argument:

    eval2 = | f a b | { f a b }
    eval2 add2 1 2                  // calls `add2 1 2`
    eval2 |a b|{a + b} 1 2          // calls anonymous function

Return a function from a function:

    sub2 = | a b | { a - b }
    choose = |x| { if (x == "add") add2 sub2 }
    choose "add" 1 2
    choose "sub" 1 2

If-condition:

    if x == "add" then add2 else sub2
    
    // possible multiline style
    if (
       x == "add"
    )
    then {
        add2 
    }
    else { 
        sub2
    }

- Spec: `if <cond> then <true-branch> else <false-branch>`
- The `else` keyword is not required, but it's good to use with longer
  then/else branches for better readability and aesthetics.
- The parens around condition are not part of syntax, but are required
  around infix operators, including comparison operators.
- If statement has otherwise same syntax as a function call. The parser
  doesn't even need to know about `if` as a keyword.

Block is a function with zero arguments:

    block1 = { c = add2 a b; }    // returns c (the semicolon is not important)
    block2 = { c = add2 a b; none }  // returns none
    block1  // evaluate the block (actually, it might have been evaluated above - that's up to compiler)
    block3 = { a + b }      // block with free variables: a, b
    block3      // the value is still { a + b } - variables are not bound, cannot be evaluated
    block3_bound = bind a=1 b=2 block3
    block3_bound    // returns 3
    
    a = {f = |x|{ 5 } }; f    // ERROR - block creates new scope - f is undefined outside
    a = (f = |x|{ 5 } ); f    // ok - f is declared in outer scope

Infix operators:

    // C++ style operators, with similar precedence rules
    // (exception is comparison operators)
    1 + 2 * 3 ** 4 == 1 + (2 * (3 ** 4))
    // Bitwise operators
    1 | 2 & 3 >> 1 == 1 | (2 & (3 >> 1))

The BitwiseOr operator has lower precedence that lambda operator:

    a | b | c       // ok, bitwise
    a | b | {c}     // calls 'a' with lambda '|b|{c}' as an argument
    a | b | ({c})   // now the '{c}' block is an arg in bitwise-or

Record field lookup:

    MyRecord = (String name, Int32 age) 
    rec = MyRecord("A name", 42)
    rec.name    # dot operator


Types
-----

Primitive types:

    12                  // Int32
    12:Int64            // Int64
    1.2                 // Float32
    1.2:Float64         // Floaf64
    true    false       // Bool
    'a'                 // Char
    "Hello."            // String = [Char]
    ("Hello", 33)       // (String, Int32)  -- a tuple
    [1, 2, 3]           // [Int32]          -- a list

Custom types are made by composition of other types.
Types must begin with uppercase letter (this is enforced part of the syntax):

    MyType = Int32
    MyTuple = (String, Int32)
    MyRecord = (String name, Int32 age)    // just a tuple with named fields
    MyBool = false | true
    MyVariant = int Int32 | string String | none   // discriminated variant type
    MyOptional = some v | none

Function types:

    |a:Int32 b:Int32| -> |c:Int32| -> Int32
    |Int32 Int32| -> |Int32| -> Int32           // without parameter names
    |Int32 Int32 Int32| -> Int32                // Compact form
    |Int32| -> |Int32| -> |Int32| -> Int32      // Normalized form

- All of the above types are equivalent - they all describe the same function.
- The normalized form describes how the partial evaluation works.
- But any of the above might describe what is really happening after compilation
  (it depends only on the compiler how many intermediate functions it creates). 


### Lists

Lists are homogenous data types:

    nums = [1, 2, 3, 4, 5]
    chars = ['a', 'b', 'c', 'd', 'e']

List of chars is equivalent to a string.

Basic operations:

    len nums == 5
    empty nums == false
    
    head nums == 1
    tail nums == [2, 3, 4, 5]
    last nums == 5
    init nums == [1, 2, 3, 4]
    
    take 3 nums == [1, 2, 3]
    take 10 nums == [1, 2, 3, 4, 5]
    drop 3 nums == [4, 5]
    drop 10 nums == []
    
    reverse nums == [5, 4, 3, 2, 1]
    min nums == 1
    max nums == 5
    sum nums == 15

Subscript (index) operator:

    # zero-based index
    nums[3] == 4

Concatenation:

    cat nums [6, 7]
    # =>  [1, 2, 3, 4, 5, 6, 7]
    cat "hello" [' '] "world"
    # =>  "hello world"
    cons 0 nums
    # =>  [0, 1, 2, 3, 4, 5]

Ranges:

    [1..10] == [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
    ['a'..'z']

Comprehensions:

    [2*x for x in [1..10] if x > 3]
    [2*x | x <= [1..10], x > 3]


Modules
-------

A top-level translation unit is named Module.
Module-level statements are either Declarations or Invocations.
Declaration can be written in any order, each name can be used only once in a scope.

Invocations are order dependent - when executing the Module, each Invocation is evaluated
and its result is passed to Executor, which is special function (possibly hardcoded in C++)
which gets a result from each Invocation, processes it and passes another value to next Invocation.
This value can be accessed inside the Invocation under special name: `_`.

Given this source file:

    1 + 2
    3 * _

Imagine that it's executed like this:

    _0 = void
    _1 = executor (|_|{ 1 + 2 } _0)
    _2 = executor (|_|{ 3 * _ } _1)

The Executor can do anything with the results, for example:

- print them to the console (i.e. just printing the program output)
- interpret them as drawing commands (i.e. implementing something similar to PostScript)
- test them for a condition (i.e. unit testing)
- concatenate them as a HTTP response (i.e. Web application)
- implementing anything else that needs a sequence of records

Importing modules:

    my_mod = import "my_mod"    // import only Declarations
    my_mod.func                 // run function imported from module `my_mod`
    my_mod                      // run all associated Invocations

- In the last line, the whole module is executed.
- The first Invocation from the module gets current '_' value.
- The statement returns the result of last Invocation in the module.

Module names must be valid function names, i.e. start with lower case letter.

Module import paths are configurable, by passing `-I` option to compiler,
by setting them in config file or via C++ interface.

All configured paths are searched in order (which yet needs to be defined),
checking for existence of source file or bitcode file:

- Source file pattern: `<import_path>/<requested_name>.ls`
- Bitcode file pattern: `<import_path>/<requested_name>.lsB`
- `<import_path>` is one of paths specified by `-I` etc.
- `<requested_name>` is the string from `import` statement, without quotes (it may contain slashes, e.g. `"lib/mod"`)
- the file extension might be configurable too, especially in the embedding scenario

The bitcode is not autogenerated - if not found, it will be created on-the-fly and stored temporarily
in local cache.


Compilation
-----------

The complete program is composed from main source plus all imported modules,
each of which is precompiled into bitcode. After this phase, we have a big blob of bitcode.

The whole precompiled blob is then passed either to interpreter, which can execute it as is,
or to a low-level compiler, for example LLVM. It just needs to be translated to IR and compiled
to machine code.

The resulting machine binary will have a main entry point, which will run a default Executor
(that will be compiled in too, for efficiency).


Appendix 
--------

List of Keywords:

    else if then

Precedence table:

    (-2) |  definition        |  =
    (-1) |  condition         |  if <cond> then <tru-branch> else <false-branch>
    (-0) |  comma             |  ,
    
    1    |  logical or        |  ||
    2    |  logical and       |  &&
    3    |  comparison        |  ==  !=  <=  >=  <  >
    4    |  bitwise or, xor   |  |  ^
    5    |  bitwise and       |  &
    6    |  bitwise shift     |  <<  >>
    7    |  add, subtract     |  +  -
    8    |  multiply, divide  |  *  /  %
    9    |  power             |  **
    10   |  subscript         |  x @ y
    
    (11) |  unary ops         |  -  +  !  ~
    (12) |  function call     |  f [<arg> ...]

Higher precedence means tighter binding.

Infix operators have numbered precedence, which can be easily changed in compiler implementation.
The other precedences are hard-coded in parser grammar.
