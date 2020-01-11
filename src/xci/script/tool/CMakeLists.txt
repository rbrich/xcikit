find_package(Replxx REQUIRED)

# "Lambda Script Interpreter" tool
add_executable(lasci
    main.cpp
    cmd_parser.cpp
    BytecodeTracer.cpp
    )
target_link_libraries(lasci
    xci-core
    xci-script
    docopt::docopt
    replxx::replxx
    )
target_include_directories(lasci PRIVATE
    $<TARGET_PROPERTY:taocpp::pegtl,INTERFACE_INCLUDE_DIRECTORIES>
    )
install(TARGETS lasci EXPORT xcikit DESTINATION bin)
