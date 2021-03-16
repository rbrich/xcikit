# Fire Script compiler/interpreter
add_executable(fire
    BytecodeTracer.cpp
    Repl.cpp
    ReplCommand.cpp
    Highlighter.cpp
    main.cpp
)
target_link_libraries(fire
    xci-core
    xci-script
    taocpp::pegtl
)
install(TARGETS fire EXPORT xcikit DESTINATION bin)
