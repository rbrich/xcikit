# -------------- #
# Catch2 helpers #
# -------------- #

include(Catch)

macro(add_catch_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE Catch2::Catch2WithMain ${ARGN})
    if (EMSCRIPTEN)
        # Discovering tests doesn't work with Emscripten
        add_test(NAME ${name} COMMAND node ${name}.js)
    else()
        catch_discover_tests(${name})
    endif()
    install(TARGETS ${name} EXPORT xcikit DESTINATION tests)
endmacro()


# ------------- #
#     Tests     #
# ------------- #

add_catch_test(test_core test_core.cpp xci-core)
if (EMSCRIPTEN)
    target_link_options(test_core PRIVATE --embed-file=${XCIKIT_BINARY_DIR}/include/xci/config.h@test_file)
    add_link_options(-sEXIT_RUNTIME=1)
endif()

add_catch_test(test_geometry test_geometry.cpp xci-core)
add_catch_test(test_chunked_stack test_chunked_stack.cpp xci-core)
add_catch_test(test_indexed_map test_indexed_map.cpp xci-core)
add_catch_test(test_argparser test_argparser.cpp xci-core)
add_catch_test(test_edit_buffer test_edit_buffer.cpp xci-core)

if (NOT EMSCRIPTEN)
    add_catch_test(test_eventloop test_eventloop.cpp xci-core)
endif()

if (XCI_DATA)
    add_catch_test(test_data test_data.cpp xci-data)
    add_catch_test(test_data_binary test_data_binary.cpp xci-data)
    add_catch_test(test_coding test_coding.cpp xci-data)
endif()

if (XCI_SCRIPT)
    add_catch_test(test_script test_script.cpp xci-script)
    if (EMSCRIPTEN)
        target_link_options(test_script PRIVATE --embed-file=${XCIKIT_BINARY_DIR}/share/xcikit/script@share/script)
    endif()
endif()

if (XCI_GRAPHICS)
    add_catch_test(test_graphics test_graphics.cpp xci-graphics)
endif()

if (XCI_WIDGETS)
    add_catch_test(test_widgets test_widgets.cpp xci-widgets xci-text xci-graphics)
endif()
