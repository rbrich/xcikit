find_package(Replxx REQUIRED)

add_executable(demo_script
    demo_script.cpp
    repl/cmd_parser.cpp
    )
target_link_libraries(demo_script
    xci-core
    xci-script
    docopt::docopt
    replxx::replxx
    )
target_include_directories(demo_script PRIVATE
    $<TARGET_PROPERTY:taocpp::pegtl,INTERFACE_INCLUDE_DIRECTORIES>
    )
install(TARGETS demo_script EXPORT xcikit DESTINATION ${INST_EXAMPLES})
