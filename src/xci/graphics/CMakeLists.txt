# ------------ #
# Dependencies #
# ------------ #

find_package(Vulkan REQUIRED)

# ------------ #
# Libraries    #
# ------------ #

add_library(xci-graphics)
add_library(xcikit::xci-graphics ALIAS xci-graphics)


target_sources(xci-graphics
    PRIVATE
        Color.cpp
        Primitives.cpp
        Renderer.cpp
        Shader.cpp
        Sprites.cpp
        Texture.cpp
        View.cpp
        Window.cpp
        shape/Ellipse.cpp
        shape/Line.cpp
        shape/Polygon.cpp
        shape/Rectangle.cpp
        shape/RoundedRectangle.cpp
        shape/Shape.cpp
        shape/Triangle.cpp
        vulkan/Attachments.cpp
        vulkan/Buffer.cpp
        vulkan/CommandBuffers.cpp
        vulkan/DescriptorPool.cpp
        vulkan/DeviceMemory.cpp
        vulkan/Framebuffer.cpp
        vulkan/Image.cpp
        vulkan/Pipeline.cpp
        vulkan/Sampler.cpp
        vulkan/Swapchain.cpp
    PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/src
    FILES
        Color.h
        Primitives.h
        Renderer.h
        Shader.h
        Sprites.h
        Texture.h
        View.h
        Window.h
        shape/Ellipse.h
        shape/Line.h
        shape/Polygon.h
        shape/Rectangle.h
        shape/RoundedRectangle.h
        shape/Shape.h
        shape/Triangle.h
        vulkan/Attachments.h
        vulkan/Buffer.h
        vulkan/CommandBuffers.h
        vulkan/DescriptorPool.h
        vulkan/DeviceMemory.h
        vulkan/Framebuffer.h
        vulkan/Image.h
        vulkan/Pipeline.h
        vulkan/Sampler.h
        vulkan/Swapchain.h
        vulkan/VulkanError.h
)

target_include_directories(xci-graphics
    PUBLIC
        ${XCI_INCLUDE_DIRS}
)

target_link_libraries(xci-graphics
    PUBLIC
        xci-core
        xci-vfs
        ${CMAKE_DL_LIBS}
        Vulkan::Vulkan
        SDL3::SDL3
    PRIVATE
        range-v3::range-v3
)
add_dependencies(xci-graphics shaders)

if (BUILD_SHARED_LIBS)
    set_target_properties(xci-graphics PROPERTIES CXX_VISIBILITY_PRESET default)
endif()

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

if (BUILD_SHARED_LIBS OR XCI_INSTALL_DEVEL)
    install(TARGETS xci-graphics
        EXPORT xcikit
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()
