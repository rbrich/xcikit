set(DEMO_DEST examples/plugin)
add_demo(demo_plugin LINK xci-core ${CMAKE_DL_LIBS})

add_library(pluggable MODULE pluggable.cpp)
target_link_libraries(pluggable xci-core)
install(TARGETS pluggable DESTINATION examples/plugin)
