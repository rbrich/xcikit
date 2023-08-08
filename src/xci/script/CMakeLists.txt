# ------------ #
# Libraries    #
# ------------ #

add_library(xci-script
    ast/AST.cpp
    ast/fold_const_expr.cpp
    ast/fold_dot_call.cpp
    ast/fold_paren.cpp
    ast/fold_tuple.cpp
    ast/resolve_decl.cpp
    ast/resolve_spec.cpp
    ast/resolve_nonlocals.cpp
    ast/resolve_symbols.cpp
    ast/resolve_types.cpp
    code/assembly_helpers.cpp
    code/optimize_copy_drop.cpp
    code/optimize_tail_call.cpp
    parser/raw_string.cpp
    typing/TypeChecker.cpp
    typing/generic_resolver.cpp
    typing/overload_resolver.cpp
    typing/type_index.cpp
    Builtin.cpp
    Class.cpp
    Code.cpp
    CodeAssembly.cpp
    Compiler.cpp
    Error.cpp
    Function.cpp
    Heap.cpp
    Interpreter.cpp
    Machine.cpp
    Module.cpp
    ModuleManager.cpp
    Value.cpp
    Parser.cpp
    Source.cpp
    Stack.cpp
    Stream.cpp
    SymbolTable.cpp
    TypeInfo.cpp
    dump.cpp)
add_library(xcikit::xci-script ALIAS xci-script)

target_precompile_headers(xci-script REUSE_FROM xci-core)

target_link_libraries(xci-script
    PUBLIC
        xci-core
        xci-core-vfs
        xci-data
    PRIVATE
        range-v3::range-v3
    )

target_include_directories(xci-script
    PUBLIC
        ${XCI_INCLUDE_DIRS}
    PRIVATE
        $<TARGET_PROPERTY:taocpp::pegtl,INTERFACE_INCLUDE_DIRECTORIES>
    )

if (BUILD_SHARED_LIBS)
    set_target_properties(xci-script PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-script EXPORT xcikit DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
