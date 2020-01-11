find_package(Replxx REQUIRED)

# "Lambda Script Interpreter" tool
add_executable(lasci
    BytecodeTracer.cpp
    Context.cpp
    ReplCommand.cpp
    main.cpp
    )
target_link_libraries(lasci
    xci-core
    xci-script
    docopt::docopt
    replxx::replxx
    )
install(TARGETS lasci EXPORT xcikit DESTINATION bin)
