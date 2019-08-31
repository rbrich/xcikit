Building with LLVM JIT
======================

First, build LLVM itself (unless using distribution packages):

    cmake -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_TARGETS_TO_BUILD=X86 \
          -DLLVM_BUILD_TOOLS=OFF -DCMAKE_INSTALL_PREFIX=<path-to>/artifacts ..

Reference:
- [LLVM CMake build](https://llvm.org/docs/CMake.html#quick-start)

