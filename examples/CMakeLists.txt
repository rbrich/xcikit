# ------------ #
# Functions    #
# ------------ #

set(EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR})

function(add_demo TARGET)
    set(options)
    set(one_value_args)
    set(multi_value_args SOURCES LINK DEPS CDEFS)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "${options}"
        "${one_value_args}" "${multi_value_args}")

    if (ARG_SOURCES)
        add_executable(${TARGET} ${ARG_SOURCES})
    else()
        add_executable(${TARGET} ${TARGET}.cpp)
    endif()

    target_include_directories(${TARGET} PRIVATE ${EXAMPLES_DIR})

    if (ARG_LINK)
        target_link_libraries(${TARGET} ${ARG_LINK})
    endif()

    if (ARG_DEPS)
        add_dependencies(${TARGET} ${ARG_DEPS})
    endif()

    if (ARG_CDEFS)
        target_compile_definitions(${TARGET} PRIVATE ${ARG_CDEFS})
    endif()

    install(TARGETS ${TARGET} EXPORT xcikit DESTINATION ${DEMO_DEST})
endfunction()


# ------------ #
# Examples     #
# ------------ #

if (EMSCRIPTEN)
    add_link_options(-sEXIT_RUNTIME=1)
endif()

# xci-core

add_subdirectory(core)

if (NOT EMSCRIPTEN)
    add_subdirectory(plugin)
endif()

# xci-data

if (XCI_DATA)
    add_subdirectory(data)
endif()

# xci-scene

if (XCI_SCENE)
    add_subdirectory(scene)
endif()

# xci-script

if (XCI_SCRIPT)
    add_subdirectory(script)
endif()

# xci-graphics

if (XCI_GRAPHICS)
    add_subdirectory(graphics)
endif()

# xci-text

if (XCI_TEXT)
    add_subdirectory(text)
endif()

# xci-widgets

if (XCI_WIDGETS)
    add_subdirectory(widgets)
endif()
