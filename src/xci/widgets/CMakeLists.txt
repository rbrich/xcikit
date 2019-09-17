# ------------ #
# Libraries    #
# ------------ #

add_library(xci-widgets
    Button.cpp
    Checkbox.cpp
    FpsDisplay.cpp
    Icon.cpp
    Theme.cpp
    Widget.cpp
    TextInput.cpp
    Form.cpp
    Label.cpp
    TextTerminal.cpp
    )
add_library(xcikit::xci-widgets ALIAS xci-widgets)

target_link_libraries(xci-widgets
    PUBLIC
        xci-text
    )

if (BUILD_SHARED_LIBS)
    set_target_properties(xci-widgets PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

if (APPLE)
    set_target_properties(xci-widgets PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup")
endif()

if (BUILD_FRAMEWORKS)
    set_target_properties(xci-widgets PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER cz.xci.widgets
        # "current version" in semantic format in Mach-O binary file
        VERSION 1.0.0
        # "compatibility version" in semantic format in Mach-O binary file
        SOVERSION 1.0.0
        PUBLIC_HEADER xci-widgets.h
        )
endif()

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-widgets EXPORT xcikit DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
