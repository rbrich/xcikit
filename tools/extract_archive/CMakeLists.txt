
# dar (DAR Archive Extractor)
add_executable(dar dar.cpp)
target_link_libraries(dar xci-vfs)
install(TARGETS dar EXPORT xcikit DESTINATION ${CMAKE_INSTALL_BINDIR})
