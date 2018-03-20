# Find Catch (https://github.com/catchorg/Catch2)
#
# optionally set CATCH_ROOT to custom root

set(CATCH_SEARCH_PATHS
    ${CATCH_ROOT})

find_path(CATCH_INCLUDE_DIR
    NAMES catch.hpp
    PATH_SUFFIXES include
    PATHS ${CATCH_SEARCH_PATHS})
mark_as_advanced(CATCH_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Catch DEFAULT_MSG
    CATCH_INCLUDE_DIR)

set(CATCH_INCLUDE_DIRS ${CATCH_INCLUDE_DIR})
