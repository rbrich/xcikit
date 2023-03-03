find_package(spirv-cross REQUIRED)

# shed (Shader Editor)
add_executable(shed shed.cpp
    CoordEditor.cpp CoordEditor.h
    ShaderCompiler.cpp ShaderCompiler.h
    UniformEditor.cpp UniformEditor.h
)

target_link_libraries(shed
    xci-graphics
    xci-widgets
    spirv-cross::spirv-cross-glsl
    range-v3::range-v3
)

install(TARGETS shed EXPORT xcikit DESTINATION ${CMAKE_INSTALL_BINDIR})
