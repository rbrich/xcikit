# Try to find glslc or glslangValidator.
# Use env VULKAN_SDK to specify custom installation root.
#
# Once done this will define:
#   GLSLC_FOUND
#   GLSLC_COMMAND (either "glslc" or "glslangValidator -V")

find_program(GLSLC_EXECUTABLE glslc
    PATHS $ENV{VULKAN_SDK}/bin)
mark_as_advanced(GLSLC_EXECUTABLE)

find_program(GLSLANG_EXECUTABLE glslangValidator
    PATHS $ENV{VULKAN_SDK}/bin)
mark_as_advanced(GLSLANG_EXECUTABLE)

if (GLSLC_EXECUTABLE)
    set(GLSLC_COMMAND ${GLSLC_EXECUTABLE})
elseif(GLSLANG_EXECUTABLE)
    set(GLSLC_COMMAND ${GLSLANG_EXECUTABLE} -V)
endif()

if (GLSLC_COMMAND)
    set(GLSLC_FOUND TRUE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLSLC DEFAULT_MSG GLSLC_COMMAND)
