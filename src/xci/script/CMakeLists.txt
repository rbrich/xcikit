# ------------ #
# Libraries    #
# ------------ #

add_library(xci-script
    ast/AST.cpp
    ast/fold_const_expr.cpp
    ast/fold_dot_call.cpp
    ast/fold_tuple.cpp
    ast/resolve_nonlocals.cpp
    ast/resolve_symbols.cpp
    ast/resolve_types.cpp
    Builtin.cpp
    Class.cpp
    Code.cpp
    Compiler.cpp
    Function.cpp
    Heap.cpp
    Interpreter.cpp
    Machine.cpp
    Module.cpp
    Value.cpp
    Parser.cpp
    Stack.cpp
    SymbolTable.cpp
    TypeInfo.cpp
    dump.cpp)
add_library(xcikit::xci-script ALIAS xci-script)

target_link_libraries(xci-script
    PUBLIC
        xci-core
    PRIVATE
        range-v3::range-v3
    )

target_include_directories(xci-script
    PUBLIC
        ${XCI_INCLUDE_DIRS}
    PRIVATE
        $<TARGET_PROPERTY:taocpp::pegtl,INTERFACE_INCLUDE_DIRECTORIES>
    )

add_dependencies(xci-script share)

install(TARGETS xci-script EXPORT xcikit DESTINATION lib)
