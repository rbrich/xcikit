find_package(LLVM CONFIG)
if (LLVM_FOUND)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}: ${LLVM_INCLUDE_DIRS}")

    llvm_map_components_to_libnames(llvm_libs
        Core ExecutionEngine Interpreter Support
        native nativecodegen
        mc mcdisassembler mcjit
        )
    message(STATUS "LLVM libs: ${llvm_libs}")

    add_executable(demo_llvm
        demo_llvm.cpp)
    target_link_libraries(demo_llvm PRIVATE
        xci-core
        ${llvm_libs})
    target_include_directories(demo_llvm PRIVATE
        ${LLVM_INCLUDE_DIRS})
    install(TARGETS demo_llvm EXPORT xcikit DESTINATION ${INST_EXAMPLES})
endif()

add_executable(demo_script demo_script.cpp)
target_link_libraries(demo_script
    xci-core xci-script
    docopt::docopt
    ${CONAN_REPLXX_ROOT}/lib/libreplxx.a)
target_include_directories(demo_script PRIVATE
    ${CONAN_REPLXX_ROOT}/include)
install(TARGETS demo_script EXPORT xcikit DESTINATION ${INST_EXAMPLES})
