find_package(GLSLC REQUIRED)

# shed (Shader Editor)
add_executable(shed shed.cpp
    compile_shader.cpp compile_shader.h)
target_link_libraries(shed
    xci-graphics
    xci-widgets)

install(TARGETS shed EXPORT xcikit DESTINATION ${CMAKE_INSTALL_BINDIR})
