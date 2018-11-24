find_package(SFML 2.4 REQUIRED system window graphics)

add_executable(demo_sfml demo_sfml.cpp)
target_link_libraries(demo_sfml
    xci-graphics-sfml)
install(TARGETS demo_sfml EXPORT xcikit DESTINATION ${INST_EXAMPLES})
