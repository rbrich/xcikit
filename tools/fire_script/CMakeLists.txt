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

target_link_libraries(fire PRIVATE
    xci-core
    xci-script
    taocpp::pegtl
    range-v3::range-v3
)

if (EMSCRIPTEN)
    target_link_options(fire PRIVATE
        --bind --closure=1 -sMODULARIZE=1 -sEXPORT_NAME=createModule
        -lidbfs.js --pre-js=${CMAKE_SOURCE_DIR}/extras/fire_script_js/pre.js
        --preload-file=${XCIKIT_BINARY_DIR}/share/xcikit/script@share/script)
    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/fire.wasm
            ${CMAKE_CURRENT_BINARY_DIR}/fire.data
        DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(TARGETS fire EXPORT xcikit DESTINATION ${CMAKE_INSTALL_BINDIR})
