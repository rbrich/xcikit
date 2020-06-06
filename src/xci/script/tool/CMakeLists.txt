find_package(Replxx REQUIRED)

# Fire Script compiler/interpreter
add_executable(fire
    BytecodeTracer.cpp
    Context.cpp
    ReplCommand.cpp
    main.cpp
    )
target_link_libraries(fire
    xci-core
    xci-script
    Replxx::replxx
    )
install(TARGETS fire EXPORT xcikit DESTINATION bin)
