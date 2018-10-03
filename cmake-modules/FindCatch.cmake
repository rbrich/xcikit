# Find Catch (https://github.com/catchorg/Catch2)
#
# Set Catch_ROOT to custom Catch installation root.
#
# Output:
#   Catch_FOUND
#   Catch_INCLUDE_DIRS
#   Catch::Catch target

find_path(CATCH_INCLUDE_DIR
    NAMES catch.hpp
    PATHS ${Catch_ROOT}
    PATH_SUFFIXES include)
mark_as_advanced(CATCH_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Catch DEFAULT_MSG
    CATCH_INCLUDE_DIR)

if (Catch_FOUND)
    set(Catch_INCLUDE_DIRS ${CATCH_INCLUDE_DIR})
endif()

if (Catch_FOUND AND NOT TARGET Catch::Catch)
    add_library(Catch::Catch INTERFACE IMPORTED)
    set_target_properties(Catch::Catch PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Catch_INCLUDE_DIRS}")
endif()
