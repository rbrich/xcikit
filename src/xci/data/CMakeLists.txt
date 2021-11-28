# ------------ #
# Dependencies #
# ------------ #

if (NOT EMSCRIPTEN)
    find_package(ZLIB REQUIRED)
endif()

# Prefer Boost, which may be preinstalled in system
# Fallback to PFR from Conan
find_package(Boost 1.75.0)
if (Boost_FOUND)
    set_target_properties(Boost::headers PROPERTIES IMPORTED_GLOBAL TRUE)
    add_library(pfr::pfr ALIAS Boost::headers)
else()
    find_package(pfr REQUIRED)
endif()


# ------------ #
# Libraries    #
# ------------ #

add_library(xci-data
    BinaryWriter.cpp
    BinaryReader.cpp
    Dumper.cpp
    Crc32.cpp
    )
add_library(xcikit::xci-data ALIAS xci-data)

target_link_libraries(xci-data
    PUBLIC
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

if (MSVC)
    # Fix passing __VA_ARGS__ between macros.
    # Old behaviour was to pass all args in single arg, ignoring commas.
    # https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
    target_compile_options(xci-data PUBLIC /Zc:preprocessor)
endif()

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-data EXPORT xcikit DESTINATION lib)
endif()
