# ------------ #
# Libraries    #
# ------------ #

add_library(xci-widgets)
add_library(xcikit::xci-widgets ALIAS xci-widgets)

target_sources(xci-widgets
    PRIVATE
        Button.cpp
        Checkbox.cpp
        ColorPicker.cpp
        Dialog.cpp
        Form.cpp
        FpsDisplay.cpp
        Icon.cpp
        Label.cpp
        Spinner.cpp
        TextInput.cpp
        TextTerminal.cpp
        Theme.cpp
        Widget.cpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/src
    FILES
        Button.h
        Checkbox.h
        ColorPicker.h
        Dialog.h
        Form.h
        FpsDisplay.h
        Icon.h
        Label.h
        Spinner.h
        TextInput.h
        TextTerminal.h
        Theme.h
        Widget.h
    )

target_link_libraries(xci-widgets
    PUBLIC
        xci-text
    PRIVATE
        SDL3::SDL3
        range-v3::range-v3
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
    install(TARGETS xci-widgets
        EXPORT xcikit
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
