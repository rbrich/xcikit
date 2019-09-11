Dynamic Code Generation
=======================

Overview of open-source libraries
---------------------------------

GNU lightning
- translates RISC-like machine code to target machine
- not optimizing
- x86_64, x86, ARM, ...

libjit
- uses C API as IR
- some optimizations
- x86_64, x86, ARM, ...

llvm, libgccjit
- big compilers, IR to machine
- optimizing, can generate SIMD 
- all possible targets

Other libs:
- https://github.com/dPgn/codegen
- Webkit B3+Air

TODO comparison:
- simple examples with different libraries
    - math in loop (SIMD optimizable)
    - branching
    - calling other functions, calling libc
