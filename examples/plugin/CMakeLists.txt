add_executable(demo_plugin demo_plugin.cpp)
target_link_libraries(demo_plugin xci-core ${CMAKE_DL_LIBS})
install(TARGETS demo_plugin EXPORT xcikit DESTINATION examples/plugin)

add_library(pluggable MODULE pluggable.cpp)
target_link_libraries(pluggable xci-core)
install(TARGETS pluggable DESTINATION examples/plugin)
