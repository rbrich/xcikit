project(cmake_wrapper)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

if(MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4309")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4309")
endif()

add_subdirectory("source_subfolder")
