# Find PEGTL (https://github.com/taocpp/PEGTL)
#
# optionally set PEGTL_ROOT to custom installation dir

set(PEGTL_SEARCH_PATHS
    ${PEGTL_ROOT})

find_path(PEGTL_INCLUDE_DIR
    NAMES tao/pegtl.hpp
    PATH_SUFFIXES include
    PATHS ${PEGTL_SEARCH_PATHS})
mark_as_advanced(PEGTL_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PEGTL DEFAULT_MSG
    PEGTL_INCLUDE_DIR)

set(PEGTL_INCLUDE_DIRS ${PEGTL_INCLUDE_DIR})
