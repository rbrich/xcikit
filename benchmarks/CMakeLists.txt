# ------------ #
# Functions    #
# ------------ #

function(add_benchmark TARGET)
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

    target_link_libraries(${TARGET} benchmark::benchmark)
    if (ARG_LINK)
        target_link_libraries(${TARGET} ${ARG_LINK})
    endif()

    if (ARG_DEPS)
        add_dependencies(${TARGET} ${ARG_DEPS})
    endif()

    if (ARG_CDEFS)
        target_compile_definitions(${TARGET} PRIVATE ${ARG_CDEFS})
    endif()

    if (XCI_INSTALL_BENCHMARKS)
        install(TARGETS ${TARGET} EXPORT xcikit DESTINATION benchmarks)
    endif()
endfunction()



# ------------ #
# Benchmarks   #
# ------------ #

add_benchmark(bm_core LINK xci-core)
add_benchmark(bm_chunked_stack LINK xci-core)
if (XCI_SCRIPT)
    add_benchmark(bm_script LINK xci-script)
endif()
