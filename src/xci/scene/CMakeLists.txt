# ------------ #
# Libraries    #
# ------------ #

add_library(xci-scene)
add_library(xcikit::xci-scene ALIAS xci-scene)


target_sources(xci-scene
    PRIVATE
        ObjParser.cpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/src
    FILES
        ObjParser.h
    )

target_compile_features(xci-scene PUBLIC cxx_std_${XCI_CXX_STANDARD})

target_link_libraries(xci-scene
    PUBLIC xci-core
    PRIVATE $<COMPILE_ONLY:taocpp::pegtl>
    )
target_include_directories(xci-scene
    PUBLIC
        ${XCI_INCLUDE_DIRS}
    )

if (BUILD_SHARED_LIBS)
    set_target_properties(xci-scene PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-scene
        EXPORT xcikit
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
