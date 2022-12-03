# ------------ #
# Dependencies #
# ------------ #

find_package(Hyperscan REQUIRED)


# ------------ #
#    Tools     #
# ------------ #

# ff (Find File)
add_executable(ff ff.cpp)
target_link_libraries(ff xci-core fmt::fmt Hyperscan::hs)
install(TARGETS ff EXPORT xcikit DESTINATION ${CMAKE_INSTALL_BINDIR})
