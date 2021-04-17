add_subdirectory(widechar_width)

if (XCI_TEXT)
    # RectangleBinPack (xci-text)
    add_subdirectory(rbp)
endif()

if (XCI_BUILD_EXAMPLES AND NOT WIN32 AND NOT EMSCRIPTEN)
    find_path(incbin_INCLUDE_DIR
        NAMES incbin.h
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/incbin)
    mark_as_advanced(incbin_INCLUDE_DIR)
    if (incbin_INCLUDE_DIR)
        message(STATUS "Found incbin: ${incbin_INCLUDE_DIR}")
        set(incbin_FOUND TRUE CACHE BOOL "Found incbin.h")
        mark_as_advanced(incbin_FOUND)
        add_library(incbin INTERFACE)
        set_target_properties(incbin PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${incbin_INCLUDE_DIR}")
        add_library(incbin::incbin ALIAS incbin)
    endif()
endif()
