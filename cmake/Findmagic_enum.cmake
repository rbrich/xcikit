# Try to find magic_enum headers.
# Use magic_enum_ROOT_DIR to specify custom installation root.
#
# Once done this will define:
#   magic_enum_FOUND
#   magic_enum_INCLUDE_DIRS
#   magic_enum::magic_enum (target)

find_path(magic_enum_INCLUDE_DIR
    NAMES magic_enum.hpp
    PATHS ${magic_enum_ROOT_DIR})
mark_as_advanced(magic_enum_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(magic_enum DEFAULT_MSG
    magic_enum_INCLUDE_DIR)

if (magic_enum_FOUND)
    set(magic_enum_INCLUDE_DIRS ${magic_enum_INCLUDE_DIR})
    if (NOT TARGET magic_enum::magic_enum)
        add_library(magic_enum::magic_enum INTERFACE IMPORTED)
        set_target_properties(magic_enum::magic_enum PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${magic_enum_INCLUDE_DIRS}")
    endif()
endif()
