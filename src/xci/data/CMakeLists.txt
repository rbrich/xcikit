# ------------ #
# Dependencies #
# ------------ #

find_package(ZLIB REQUIRED)
find_package(pfr REQUIRED)


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
    PRIVATE
        ZLIB::ZLIB
    PUBLIC
        pfr::pfr
        magic_enum::magic_enum
    )

target_include_directories(xci-data
    PUBLIC
        ${CMAKE_SOURCE_DIR}/third_party
        ${XCI_INCLUDE_DIRS}
    )

if (MSVC)
    # Fix passing __VA_ARGS__ between macros.
    # Old behaviour was to pass all args in single arg, ignoring commas.
    # https://stackoverflow.com/questions/5134523/msvc-doesnt-expand-va-args-correctly
    target_compile_options(xci-data PUBLIC /Zc:preprocessor)
endif()

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    #install(TARGETS xci-data EXPORT xcikit DESTINATION lib)
endif()
