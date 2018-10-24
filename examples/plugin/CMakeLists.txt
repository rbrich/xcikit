add_executable(demo_plugin demo_plugin.cpp)
target_link_libraries(demo_plugin xci-core ${CMAKE_DL_LIBS})
target_compile_definitions(demo_plugin PRIVATE
    XCI_DEMO_PLUGINS="${CMAKE_INSTALL_PREFIX}/share/xcikit/examples")

add_library(pluggable MODULE pluggable.cpp)
target_link_libraries(pluggable xci-core)
install(TARGETS pluggable DESTINATION share/xcikit/examples)
