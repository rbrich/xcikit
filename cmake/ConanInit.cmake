option(RUN_CONAN "Run 'conan install' from CMake (this may be more convenient than separate command)" OFF)

# Run conan install directly
if (RUN_CONAN)
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.15/conan.cmake"
            "${CMAKE_BINARY_DIR}/conan.cmake"
            TLS_VERIFY ON
            LOG dl_log
            STATUS dl_status)
        if (NOT 0 IN_LIST dl_status)
            file(REMOVE "${CMAKE_BINARY_DIR}/conan.cmake")
            message(STATUS "Download: ${dl_log}")
            message(SEND_ERROR "Download failed: ${dl_status}")
        endif()
    endif()

    include(${CMAKE_BINARY_DIR}/conan.cmake)

    if (APPLE)
        conan_cmake_run(
            CONANFILE conanfile.py
            SETTINGS os.version=${CMAKE_OSX_DEPLOYMENT_TARGET}
            BUILD missing)
    else()
        conan_cmake_run(
            CONANFILE conanfile.py
            BUILD missing)
    endif()
endif()

# Enable lookup for Conan dependencies
if (EXISTS ${CMAKE_BINARY_DIR}/conan_paths.cmake)
    include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
endif()
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
