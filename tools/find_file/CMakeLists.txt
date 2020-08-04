# ------------ #
# Dependencies #
# ------------ #

find_package(Hyperscan)


# ------------ #
#    Tools     #
# ------------ #

# ff (Find File)
if (Hyperscan_FOUND)
    add_executable(ff ff.cpp)
    target_link_libraries(ff xci-core fmt::fmt Hyperscan::hs)
    install(TARGETS ff EXPORT xcikit DESTINATION bin)
endif()
