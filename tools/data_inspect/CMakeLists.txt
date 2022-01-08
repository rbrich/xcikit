
# dati (Data Inspector)
add_executable(dati data_inspect.cpp)
target_link_libraries(dati xci-data xci-core fmt::fmt)
install(TARGETS dati EXPORT xcikit DESTINATION bin)
