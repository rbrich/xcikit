find_package(SFML 2.5 REQUIRED COMPONENTS system window graphics)

add_executable(demo_sfml demo_sfml.cpp)
target_link_libraries(demo_sfml
    xci-text
    xci-graphics-opengl
    sfml-graphics)
install(TARGETS demo_sfml EXPORT xcikit DESTINATION ${INST_EXAMPLES})
