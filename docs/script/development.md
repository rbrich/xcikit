## Test suite

Build `test_script` and run it to check if new code broke something.
Add test cases for new code to `test_script.cpp`.

## Fixing or modifying the compiler

Build and run `fire -b -t -e "EXPR"`, where `EXPR` is a minimal fragment of the script to test new feature or reproduce a bug.
This command outputs "Processed AST" and "Bytecode trace".
Use Processed AST to understand how the code translates to AST nodes and how the compiler walks through them in each compilation pass.
Use Bytecode trace to understand the bytecode that was generated and how it runs.

To further **minimize the scope**, add `-p PASS_LIST`, e.g. `-p resolve_spec`, to stop after the named pass.
It still brings in some previous passes on which it depends. No bytecode will be generated, so `-b` won't work in this case.

Another alternative is **REPL mode** - just leave out the `-e` parameter.
The AST and bytecode dumping still works, it will be printed for each evaluated input.
Note that each input creates new module, which is then imported by next input. This can uncover bugs that are not seen in single-input `-e` expressions.

## Binary location inside `cmake-build-debug`

* `fire` is `tools/fire_script/fire`
* `test_script` is `tests/test_script`

Build targets for CMake and ninja are `fire`, `test_script`.
