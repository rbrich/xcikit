# -------------- #
# Catch2 helpers #
# -------------- #

include(CTest)
include(Catch)

add_library(catch_main STATIC catch_main.cpp)
target_link_libraries(catch_main Catch2::Catch2)
target_compile_definitions(catch_main PUBLIC CATCH_CONFIG_FAST_COMPILE)

macro(add_catch_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} catch_main ${ARGN})
    # Catch2 in PCH
    if (XCI_PCH_CATCH2)
        if (DEFINED target_with_pch)
            # already have PCH in another target, reuse it
            target_precompile_headers(${name} REUSE_FROM ${target_with_pch})
        else()
            # no PCH yet, create it and save this target name
            target_precompile_headers(${name} PRIVATE <catch2/catch.hpp>)
            set(target_with_pch ${name})
        endif()
    endif()
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
    target_link_options(test_core PRIVATE --embed-file=${CMAKE_BINARY_DIR}/include/xci/config.h@test_file)
endif()

add_catch_test(test_geometry test_geometry.cpp xci-core)
add_catch_test(test_chunked_stack test_chunked_stack.cpp xci-core)
add_catch_test(test_argparser test_argparser.cpp xci-core)

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
        target_link_options(test_script PRIVATE --embed-file=${CMAKE_BINARY_DIR}/share/script@share/script)
    endif()
endif()

if (XCI_GRAPHICS)
    add_catch_test(test_graphics test_graphics.cpp xci-graphics)
endif()

if (XCI_WIDGETS)
    add_catch_test(test_widgets test_widgets.cpp xci-widgets xci-text xci-graphics)
endif()
