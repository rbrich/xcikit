# ------------ #
# Libraries    #
# ------------ #

add_library(xci-script)
add_library(xcikit::xci-script ALIAS xci-script)

target_sources(xci-script
    PRIVATE
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
        NameId.cpp
        Value.cpp
        Parser.cpp
        Source.cpp
        Stack.cpp
        Stream.cpp
        SymbolTable.cpp
        TypeInfo.cpp
        dump.cpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/src
    FILES
        ast/AST.h
        ast/AST_serialization.h
        ast/fold_const_expr.h
        ast/fold_dot_call.h
        ast/fold_paren.h
        ast/fold_tuple.h
        ast/resolve_decl.h
        ast/resolve_spec.h
        ast/resolve_nonlocals.h
        ast/resolve_symbols.h
        ast/resolve_types.h
        code/assembly_helpers.h
        code/optimize_copy_drop.h
        code/optimize_tail_call.h
        typing/TypeChecker.h
        typing/generic_resolver.h
        typing/overload_resolver.h
        typing/type_index.h
        Builtin.h
        Class.h
        Code.h
        CodeAssembly.h
        Compiler.h
        Error.h
        Function.h
        Heap.h
        Interpreter.h
        Machine.h
        Module.h
        ModuleManager.h
        NameId.h
        NativeDelegate.h
        Value.h
        Parser.h
        Source.h
        Stack.h
        Stream.h
        SymbolTable.h
        TypeInfo.h
        dump.h
)

target_precompile_headers(xci-script REUSE_FROM xci-core)

target_link_libraries(xci-script
    PUBLIC
        xci-core
        xci-vfs
        xci-data
    PRIVATE
        range-v3::range-v3
        $<COMPILE_ONLY:taocpp::pegtl>
    )

target_include_directories(xci-script
    PUBLIC
        ${XCI_INCLUDE_DIRS}
    )

if (BUILD_SHARED_LIBS)
    set_target_properties(xci-script PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-script
        EXPORT xcikit
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
