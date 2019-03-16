add_library(xci-data
    BinaryWriter.cpp
    BinaryReader.cpp
    reflection.cpp
    )

target_link_libraries(xci-data
    PRIVATE
        ZLIB::ZLIB
    )

target_include_directories(xci-data
    PUBLIC
        ${CMAKE_SOURCE_DIR}/ext
        ${XCI_INCLUDE_DIRS}
    )