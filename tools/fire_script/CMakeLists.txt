# Fire Script compiler/interpreter
add_executable(fire
    BytecodeTracer.cpp
    Options.cpp
    Program.cpp
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
    target_link_options(fire PRIVATE
        --bind --closure=1 -sMODULARIZE=1 -sEXPORT_NAME=fire_script
        --embed-file=${CMAKE_BINARY_DIR}/share/script@share/script)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fire.wasm DESTINATION bin)
endif()

install(TARGETS fire EXPORT xcikit DESTINATION bin)
