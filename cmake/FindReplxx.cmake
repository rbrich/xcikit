# Try to find Replxx.
# Use Replxx_ROOT_DIR to specify custom installation dir.
#
# Once done this will define:
#   Replxx_FOUND
#   Replxx_INCLUDE_DIRS
#   Replxx_LIBRARIES
#   Replxx::replxx target

find_path(Replxx_INCLUDE_DIR
    NAMES replxx.hxx
    PATHS ${Replxx_ROOT_DIR})
mark_as_advanced(Replxx_INCLUDE_DIR)

find_library(Replxx_LIBRARY
    NAMES replxx replxx-d replxx-static replxx-static-d
    PATHS ${Replxx_ROOT_DIR})
mark_as_advanced(Replxx_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Replxx DEFAULT_MSG
    Replxx_LIBRARY Replxx_INCLUDE_DIR)

if (Replxx_FOUND)
    set(Replxx_LIBRARIES ${Replxx_LIBRARY})
    set(Replxx_INCLUDE_DIRS ${Replxx_INCLUDE_DIR})
    if (NOT TARGET Replxx::replxx)
        add_library(Replxx::replxx INTERFACE IMPORTED)
        set_target_properties(Replxx::replxx PROPERTIES
            INTERFACE_LINK_LIBRARIES "${Replxx_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${Replxx_INCLUDE_DIRS}"
            INTERFACE_COMPILE_DEFINITIONS "REPLXX_STATIC")
    endif()
endif()
