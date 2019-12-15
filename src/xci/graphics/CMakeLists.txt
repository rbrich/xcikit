# ------------ #
# Dependencies #
# ------------ #

find_package(glfw3 3.2 REQUIRED)
find_package(Vulkan REQUIRED)


# ------------ #
# Libraries    #
# ------------ #

add_library(xci-graphics
    Color.cpp
    Primitives.cpp
    Renderer.cpp
    Shader.cpp
    Shape.cpp
    Sprites.cpp
    Texture.cpp
    View.cpp
    Window.cpp
    vulkan/CommandBuffer.cpp
    vulkan/DeviceMemory.cpp
    )

target_include_directories(xci-graphics
    PUBLIC
        ${XCI_INCLUDE_DIRS}
    PRIVATE
        ${CONAN_INCLUDE_DIRS_INCBIN})

target_link_libraries(xci-graphics
    PUBLIC
        xci-core
        ${CMAKE_DL_LIBS}
        Vulkan::Vulkan
    PRIVATE
        range-v3
        glfw)

if (BUILD_FRAMEWORKS)
    set_target_properties(xci-graphics PROPERTIES
        FRAMEWORK TRUE
        FRAMEWORK_VERSION A
        MACOSX_FRAMEWORK_IDENTIFIER cz.xci.graphics
        # "current version" in semantic format in Mach-O binary file
        VERSION 1.0.0
        # "compatibility version" in semantic format in Mach-O binary file
        SOVERSION 1.0.0
        PUBLIC_HEADER xci-graphics.h
        )
endif()

install(TARGETS xci-graphics EXPORT xcikit DESTINATION lib)
