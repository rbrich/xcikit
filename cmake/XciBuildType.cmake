# I'd like to have Release as default build type on all platforms, so it's consistent with Conan.
#
# Without this workaround, it was quite wild:
# * Mac, Linux: default is empty string, can be overridden using a snippet from [1]
# * Windows with MSVC: default is Debug, override is more complicated, see [2]
#
# References:
# [1] https://www.kitware.com/cmake-and-the-default-build-type/
# [2] https://gitlab.kitware.com/cmake/cmake/-/issues/19401

# set default build type to Release
get_property(IS_MULTICONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (NOT IS_MULTICONFIG AND NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build.")
endif()

# set possible build types for cmake gui
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
