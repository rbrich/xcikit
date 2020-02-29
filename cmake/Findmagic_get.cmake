# Try to find magic_get headers.
# Use magic_get_ROOT_DIR to specify custom installation root.
#
# Once done this will define:
#   magic_get_FOUND
#   magic_get_INCLUDE_DIRS
#   magic_get::magic_get (target)

find_path(magic_get_INCLUDE_DIR
    NAMES include/boost/pfr.hpp
    PATHS ${magic_get_ROOT_DIR})
mark_as_advanced(magic_get_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(magic_get DEFAULT_MSG
    magic_get_INCLUDE_DIR)

if (magic_get_FOUND)
    set(magic_get_INCLUDE_DIRS ${magic_get_INCLUDE_DIR})
    if (NOT TARGET magic_get::magic_get)
        add_library(magic_get::magic_get INTERFACE IMPORTED)
        set_target_properties(magic_get::magic_get PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${magic_get_INCLUDE_DIRS}")
    endif()
endif()
