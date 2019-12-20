add_executable(demo_script demo_script.cpp)
target_link_libraries(demo_script
    xci-core xci-script
    docopt::docopt
    ${CONAN_REPLXX_ROOT}/lib/libreplxx.a)
target_include_directories(demo_script PRIVATE
    ${CONAN_REPLXX_ROOT}/include
    $<TARGET_PROPERTY:taocpp::pegtl,INTERFACE_INCLUDE_DIRECTORIES>)
install(TARGETS demo_script EXPORT xcikit DESTINATION ${INST_EXAMPLES})
