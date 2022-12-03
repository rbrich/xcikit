# tc (Term Ctl)
add_executable(tc tc.cpp)
target_link_libraries(tc xci-core)
install(TARGETS tc EXPORT xcikit DESTINATION ${CMAKE_INSTALL_BINDIR})
