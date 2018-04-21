add_executable(demo_plugin demo_plugin.cpp)
target_link_libraries(demo_plugin ${XCI_UTIL_LIBRARY} ${CMAKE_DL_LIBS})
target_compile_definitions(demo_plugin PRIVATE
    XCI_DEMO_PLUGINS="${CMAKE_INSTALL_PREFIX}/lib")

add_library(pluggable SHARED pluggable.cpp)
target_link_libraries(pluggable ${XCI_UTIL_LIBRARY})
install(TARGETS pluggable DESTINATION lib)
