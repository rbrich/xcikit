# ------------ #
# Dependencies #
# ------------ #

find_package(Hyperscan)

# ------------ #
#   Examples   #
# ------------ #

add_executable(demo_argparser demo_argparser.cpp)
target_link_libraries(demo_argparser xci-core)
install(TARGETS demo_argparser EXPORT xcikit DESTINATION examples)

add_executable(demo_dispatch demo_dispatch.cpp)
target_link_libraries(demo_dispatch xci-core)
install(TARGETS demo_dispatch EXPORT xcikit DESTINATION examples)

add_executable(demo_event demo_event.cpp)
target_link_libraries(demo_event xci-core)
install(TARGETS demo_event EXPORT xcikit DESTINATION examples)

add_executable(demo_filewatch demo_filewatch.cpp)
target_link_libraries(demo_filewatch xci-core)
install(TARGETS demo_filewatch EXPORT xcikit DESTINATION examples)

add_executable(demo_log demo_log.cpp)
target_link_libraries(demo_log xci-core)
install(TARGETS demo_log EXPORT xcikit DESTINATION examples)

add_executable(demo_chunked_stack demo_chunked_stack.cpp)
target_link_libraries(demo_chunked_stack xci-core)
install(TARGETS demo_chunked_stack EXPORT xcikit DESTINATION examples)

add_executable(demo_termctl demo_termctl.cpp)
target_link_libraries(demo_termctl xci-core)
install(TARGETS demo_termctl EXPORT xcikit DESTINATION examples)

add_executable(demo_vfs demo_vfs.cpp)
target_link_libraries(demo_vfs xci-core)
install(TARGETS demo_vfs EXPORT xcikit DESTINATION examples)


# ff (Find File) tool
if (Hyperscan_FOUND)
    add_executable(ff ff.cpp)
    target_link_libraries(ff xci-core fmt::fmt Hyperscan::hs)
    install(TARGETS ff EXPORT xcikit DESTINATION bin)
endif()
