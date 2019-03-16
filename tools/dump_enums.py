#!/usr/bin/env python
#
# This script uses Clang Python bindings to traverse the C++ AST
# and prints out the enums.
#
# You need to export compilation database with CMake:
#
#   cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=YES
#         -DCMAKE_BUILD_TYPE=Debug
#         -DCMAKE_CXX_COMPILER=/usr/lib/llvm-6.0/bin/clang++ ..
#
# The script is far from finished state, but already works on my machine
# (that is Debian Stretch + Clang 6 from the backports).

from clang.cindex import *
import clang.cindex
import os

sources_root = os.path.realpath("..")
source_file = os.path.join(sources_root, "examples/reflect/demo_reflect.cpp")

basic_compile_args = ['-I/usr/local/include',
                      '-I/usr/lib/llvm-6.0/lib/clang/6.0.0/include',
                      '-I/usr/include/x86_64-linux-gnu',
                      '-I/usr/include']


def walk_ast(c):
    if c.kind.is_translation_unit():
        print "file", c.spelling
        for c in c.get_children():
            walk_ast(c)
        return

    if not c.location.file.name.startswith(sources_root):
        return

    if c.kind == CursorKind.NAMESPACE:
        #print "namespace", c.spelling
        for c in c.get_children():
            walk_ast(c)
        return

    if c.kind == CursorKind.ENUM_DECL:
        print "enum", "class" if c.is_scoped_enum() else "",\
            c.spelling, ":", c.enum_type.spelling, "{"
        for c in c.get_children():
            if c.kind == CursorKind.ENUM_CONSTANT_DECL:
                print "   ", c.spelling, "=", c.enum_value
        print "}"
        return

    #print c.kind, c.spelling, c.location.file.name


Config.set_compatibility_check(True)
Config.set_library_file("libclang-6.0.so.1")

# WORKAROUND: second arg is for printing diagnostic messages (compiler errors)
# Normal usage: Index.create()
index = Index(clang.cindex.conf.lib.clang_createIndex(1, 1))

cdb = CompilationDatabase.fromDirectory('../build')

command = cdb.getCompileCommands(source_file)[0]
print "Compile directory:", command.directory
os.chdir(command.directory)
print "Compile command:", ' '.join(command.arguments)
compile_args = [a for a in command.arguments if a.startswith('-I') or a.startswith('-std=') or a.startswith('-W')]
compile_args += basic_compile_args
print "Evaluated args:", compile_args

unit = index.parse(command.filename, args=compile_args,
                   options=TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
walk_ast(unit.cursor)
