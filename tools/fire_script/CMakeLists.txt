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
    range-v3::range-v3
)

if (EMSCRIPTEN)
    target_link_options(fire PRIVATE
        --bind --closure=1 -sMODULARIZE=1 -sEXPORT_NAME=fire_script
        --preload-file=${CMAKE_BINARY_DIR}/share/script@share/script)
    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/fire.wasm
            ${CMAKE_CURRENT_BINARY_DIR}/fire.data
        DESTINATION bin)
endif()

install(TARGETS fire EXPORT xcikit DESTINATION bin)
