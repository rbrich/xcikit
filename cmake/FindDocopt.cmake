# Try to find Docopt.
# Use docopt_ROOT_DIR to specify custom installation dir.
#
# Once done this will define:
#   docopt_FOUND
#   docopt_INCLUDE_DIRS
#   docopt_LIBRARIES
#   docopt::docopt target

find_path(docopt_INCLUDE_DIR
    NAMES docopt.h
    PATHS ${docopt_ROOT_DIR})
mark_as_advanced(docopt_INCLUDE_DIR)

find_library(docopt_LIBRARY
    NAMES docopt
    PATHS ${docopt_ROOT_DIR})
mark_as_advanced(docopt_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(docopt DEFAULT_MSG
    docopt_LIBRARY docopt_INCLUDE_DIR)

if (docopt_FOUND)
    set(docopt_LIBRARIES ${docopt_LIBRARY})
    set(docopt_INCLUDE_DIRS ${docopt_INCLUDE_DIR})
    if (NOT TARGET docopt::docopt)
        add_library(docopt::docopt INTERFACE IMPORTED)
        set_target_properties(docopt::docopt PROPERTIES
            INTERFACE_LINK_LIBRARIES "${docopt_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${docopt_INCLUDE_DIRS}")
    endif()
endif()
