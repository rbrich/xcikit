# ------------ #
# Dependencies #
# ------------ #

# Prefer Boost, which may be preinstalled in system
# Fallback to PFR from Conan
find_package(Boost 1.75.0)
if (Boost_FOUND)
    set_target_properties(Boost::headers PROPERTIES IMPORTED_GLOBAL TRUE)
    add_library(pfr::pfr ALIAS Boost::headers)
else()
    find_package(pfr QUIET)
    if (NOT pfr_FOUND)
        # vcpkg doesn't provide any cmake config, find the headers directly
        find_path(pfr_INCLUDE_DIR NAMES boost/pfr/config.hpp)
        if (pfr_INCLUDE_DIR)
            add_library(pfr::pfr INTERFACE IMPORTED)
            target_include_directories(pfr::pfr INTERFACE ${pfr_INCLUDE_DIR})
        else ()
            message(ERROR "pfr not found!")
        endif()
    endif()
endif()


# ------------ #
# Libraries    #
# ------------ #

add_library(xci-data
    BinaryWriter.cpp
    BinaryReader.cpp
    Crc32.cpp
    Dumper.cpp
    Schema.cpp
    )
add_library(xcikit::xci-data ALIAS xci-data)

target_link_libraries(xci-data
    PUBLIC
        xci-core
        pfr::pfr
        magic_enum::magic_enum
    )

if (EMSCRIPTEN)
    target_compile_options(xci-data PUBLIC -sUSE_ZLIB=1)
    target_link_options(xci-data PUBLIC -sUSE_ZLIB=1)
else()
    target_link_libraries(xci-data PRIVATE ZLIB::ZLIB)
endif()

target_include_directories(xci-data
    PUBLIC
        ${XCI_INCLUDE_DIRS}
    )

if (BUILD_SHARED_LIBS)
    set_target_properties(xci-data PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-data EXPORT xcikit DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
