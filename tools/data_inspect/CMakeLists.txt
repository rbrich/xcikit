
# dati (Data Inspector)
add_executable(dati dati.cpp)
target_link_libraries(dati xci-data xci-core)
install(TARGETS dati EXPORT xcikit DESTINATION ${CMAKE_INSTALL_BINDIR})
