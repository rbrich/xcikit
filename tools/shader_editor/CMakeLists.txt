find_package(spirv-cross REQUIRED)

# shed (Shader Editor)
add_executable(shed shed.cpp
    ShaderCompiler.cpp ShaderCompiler.h
    UniformEditor.cpp UniformEditor.h
)

target_link_libraries(shed
    xci-graphics
    xci-widgets
    spirv-cross::spirv-cross-glsl
)

install(TARGETS shed EXPORT xcikit DESTINATION ${CMAKE_INSTALL_BINDIR})
