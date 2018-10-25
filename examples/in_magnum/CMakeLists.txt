if(APPLE)
    find_package(Magnum REQUIRED CglContext)
else()
    find_package(Magnum REQUIRED GlxContext)
endif()

add_executable(demo_magnum demo_magnum.cpp)
target_link_libraries(demo_magnum
    xci-graphics-magnum
    Magnum::GLContext
    glfw)
install(TARGETS demo_magnum EXPORT xcikit DESTINATION ${INST_EXAMPLES})
