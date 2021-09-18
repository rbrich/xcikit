option(CONAN_INSTALL "Run 'conan install' from CMake (this may be more convenient than separate command)" OFF)
set(CONAN_PROFILE "default" CACHE STRING "Conan profile ot use in 'conan install'")

# Run conan install directly
# See https://github.com/conan-io/cmake-conan
if (CONAN_INSTALL)
    if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
        message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
        file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/v0.16.1/conan.cmake"
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

    macro(opt_to_conan OPT CONAN_VAL)
        if (${OPT})
            set(${CONAN_VAL} True)
        else()
            set(${CONAN_VAL} False)
        endif()
    endmacro()
    opt_to_conan(XCI_DATA OPT_DATA)
    opt_to_conan(XCI_SCRIPT OPT_SCRIPT)
    opt_to_conan(XCI_GRAPHICS OPT_GRAPHICS)
    opt_to_conan(XCI_TEXT OPT_TEXT)
    opt_to_conan(XCI_WIDGETS OPT_WIDGETS)

    conan_cmake_install(
        PATH_OR_REFERENCE ${CMAKE_CURRENT_SOURCE_DIR}
        INSTALL_FOLDER ${CMAKE_BINARY_DIR}
        PROFILE_HOST ${CONAN_PROFILE}
        PROFILE_BUILD ${CONAN_PROFILE}
        SETTINGS
            build_type=${CMAKE_BUILD_TYPE}
        OPTIONS
            xcikit:data=${OPT_DATA}
            xcikit:script=${OPT_SCRIPT}
            xcikit:graphics=${OPT_GRAPHICS}
            xcikit:text=${OPT_TEXT}
            xcikit:widgets=${OPT_WIDGETS}
        BUILD missing)
endif()
