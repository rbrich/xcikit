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

if (EMSCRIPTEN)
    target_link_options(fire PRIVATE --embed-file=${CMAKE_BINARY_DIR}/share/script@share/script)
endif()

install(TARGETS fire EXPORT xcikit DESTINATION bin)
