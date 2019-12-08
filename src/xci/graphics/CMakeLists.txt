# ------------ #
# Sources      #
# ------------ #

set(XCI_GRAPHICS_SOURCES
    Color.cpp
    Renderer.cpp
    Primitives.cpp
    Shader.cpp
    Shape.cpp
    Sprites.cpp
    View.cpp
    )

set(XCI_GRAPHICS_OPENGL_SOURCES
    opengl/GlPrimitives.cpp
    opengl/GlRenderer.cpp
    opengl/GlShader.cpp
    opengl/GlTexture.cpp
    opengl/GlWindow.cpp)

set(XCI_GRAPHICS_VULKAN_SOURCES
    vulkan/CommandBuffer.cpp
    vulkan/DeviceMemory.cpp
    vulkan/VulkanPrimitives.cpp
    vulkan/VulkanRenderer.cpp
    vulkan/VulkanShader.cpp
    vulkan/VulkanTexture.cpp
    vulkan/VulkanWindow.cpp)


# ------------ #
# Dependencies #
# ------------ #

# OpenGL / Vulkan
if (XCI_WITH_OPENGL OR XCI_WITH_VULKAN)
    find_package(glfw3 3.2 REQUIRED)
endif()
if (XCI_WITH_VULKAN)
    find_package(Vulkan REQUIRED)
endif()
if (XCI_WITH_OPENGL)
    find_package(glad REQUIRED)
endif ()


# ------------ #
# Libraries    #
# ------------ #

if (XCI_WITH_OPENGL)
    add_library(xci-graphics-opengl
        ${XCI_GRAPHICS_SOURCES} ${XCI_GRAPHICS_OPENGL_SOURCES})
    target_include_directories(xci-graphics-opengl
        PUBLIC
            ${XCI_INCLUDE_DIRS}
        PRIVATE
            ${CONAN_INCLUDE_DIRS_INCBIN})
    target_link_libraries(xci-graphics-opengl
        PUBLIC
            xci-core
            ${CMAKE_DL_LIBS}
            glad::glad
        PRIVATE
            glfw)

    if (BUILD_FRAMEWORKS)
        set_target_properties(xci-graphics-opengl PROPERTIES
            FRAMEWORK TRUE
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER cz.xci.xci-graphics-opengl
            # "current version" in semantic format in Mach-O binary file
            VERSION 1.0.0
            # "compatibility version" in semantic format in Mach-O binary file
            SOVERSION 1.0.0
            PUBLIC_HEADER xci-graphics.h
            )
    endif()

    install(TARGETS xci-graphics-opengl
        EXPORT xcikit DESTINATION lib)
endif ()

if (XCI_WITH_VULKAN)
    add_library(xci-graphics-vulkan
        ${XCI_GRAPHICS_SOURCES} ${XCI_GRAPHICS_VULKAN_SOURCES})
    target_include_directories(xci-graphics-vulkan
        PUBLIC
            ${XCI_INCLUDE_DIRS}
        PRIVATE
            ${CONAN_INCLUDE_DIRS_INCBIN})
    target_link_libraries(xci-graphics-vulkan
        PUBLIC
            xci-core
            ${CMAKE_DL_LIBS}
            Vulkan::Vulkan
        PRIVATE
            range-v3
            glfw)
    install(TARGETS xci-graphics-vulkan
        EXPORT xcikit DESTINATION lib)
endif ()
