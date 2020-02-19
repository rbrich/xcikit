project (mman-win32 C)
cmake_minimum_required(VERSION 3.9)

add_library(mman-win32 STATIC mman.c)
target_include_directories(mman-win32 INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
    $<INSTALL_INTERFACE:include>)

install (FILES mman.h DESTINATION include/mman-win32)
install(TARGETS mman-win32
    EXPORT xcikit
    DESTINATION lib
    INCLUDES DESTINATION include)
