# Try to find Hyperscan.
# Use Hyperscan_ROOT_DIR to specify custom installation dir.
#
# Once done this will define:
#   Hyperscan_FOUND
#   Hyperscan_INCLUDE_DIRS
#   Hyperscan_LIBRARIES
#   Hyperscan::Hyperscan target

find_path(Hyperscan_INCLUDE_DIR
    NAMES hs/hs.h
    PATHS ${Hyperscan_ROOT_DIR})
mark_as_advanced(Hyperscan_INCLUDE_DIR)

find_library(Hyperscan_LIBRARY
    NAMES hs
    PATHS ${Hyperscan_ROOT_DIR})
mark_as_advanced(Hyperscan_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Hyperscan DEFAULT_MSG
    Hyperscan_LIBRARY Hyperscan_INCLUDE_DIR)

if (Hyperscan_FOUND)
    set(Hyperscan_LIBRARIES ${Hyperscan_LIBRARY})
    set(Hyperscan_INCLUDE_DIRS ${Hyperscan_INCLUDE_DIR})
    if (NOT TARGET Hyperscan::hs)
        add_library(Hyperscan::hs INTERFACE IMPORTED)
        set_target_properties(Hyperscan::hs PROPERTIES
            INTERFACE_LINK_LIBRARIES "${Hyperscan_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${Hyperscan_INCLUDE_DIRS}")
    endif()
endif()
