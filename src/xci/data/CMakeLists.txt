# ------------ #
# Dependencies #
# ------------ #

find_package(ZLIB REQUIRED)
find_package(magic_get REQUIRED)
find_package(magic_enum REQUIRED)


# ------------ #
# Libraries    #
# ------------ #

add_library(xci-data
    BinaryWriter.cpp
    BinaryReader.cpp
    Dumper.cpp
    Crc32.cpp
    )

target_link_libraries(xci-data
    PRIVATE
        ZLIB::ZLIB
    PUBLIC
        magic_get::magic_get
        magic_enum::magic_enum
    )

target_include_directories(xci-data
    PUBLIC
        ${CMAKE_SOURCE_DIR}/ext
        ${XCI_INCLUDE_DIRS}
    )

#install(TARGETS xci-data EXPORT xcikit DESTINATION lib)
