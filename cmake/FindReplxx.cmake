# Try to find Replxx.
# Use replxx_ROOT_DIR to specify custom installation dir.
#
# Once done this will define:
#   replxx_FOUND
#   replxx_INCLUDE_DIRS
#   replxx_LIBRARIES
#   replxx::replxx target

find_path(replxx_INCLUDE_DIR
    NAMES replxx.hxx
    PATHS ${replxx_ROOT_DIR})
mark_as_advanced(replxx_INCLUDE_DIR)

find_library(replxx_LIBRARY
    NAMES replxx replxx-static replxx-static-d
    PATHS ${replxx_ROOT_DIR})
mark_as_advanced(replxx_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(replxx DEFAULT_MSG
    replxx_LIBRARY replxx_INCLUDE_DIR)

if (replxx_FOUND)
    set(replxx_LIBRARIES ${replxx_LIBRARY})
    set(replxx_INCLUDE_DIRS ${replxx_INCLUDE_DIR})
    if (NOT TARGET replxx::replxx)
        add_library(replxx::replxx INTERFACE IMPORTED)
        set_target_properties(replxx::replxx PROPERTIES
            INTERFACE_LINK_LIBRARIES "${replxx_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${replxx_INCLUDE_DIRS}"
            INTERFACE_COMPILE_DEFINITIONS "REPLXX_STATIC")
    endif()
endif()
