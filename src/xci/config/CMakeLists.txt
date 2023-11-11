# ------------------- #
# Library: xci-config #
# ------------------- #

add_library(xci-config
    Config.cpp
    ConfigParser.cpp
)
add_library(xcikit::xci-config ALIAS xci-config)
target_precompile_headers(xci-config REUSE_FROM xci-core)

target_link_libraries(xci-config PUBLIC xci-core)
target_include_directories(xci-config
    PRIVATE $<TARGET_PROPERTY:taocpp::pegtl,INTERFACE_INCLUDE_DIRECTORIES>
)


if (BUILD_SHARED_LIBS)
    set_target_properties(xci-config PROPERTIES CXX_VISIBILITY_PRESET default)
endif()


# ------- #
# Install #
# ------- #

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-config
        EXPORT xcikit
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
