# Find Abseil (https://github.com/abseil/abseil-cpp)
#
# Set Abseil_ROOT to custom Abseil installation root.
#
# Output:
#   Abseil_FOUND
#   Abseil_INCLUDE_DIRS
#   Abseil_LIBRARIES
#   Targets:
#       absl::base
#       absl::strings

find_path(Abseil_INCLUDE_DIRS
    NAMES absl/base/config.h
    PATHS ${Abseil_ROOT}
    PATH_SUFFIXES include)

set(Abseil_LIBS base strings throw_delegate)
set(Abseil_LIBRARIES "")

foreach(lib ${Abseil_LIBS})
    find_library(Abseil_${lib}_LIBRARY
        NAMES absl_${lib}
        PATHS ${Abseil_ROOT}
        PATH_SUFFIXES lib)
    list(APPEND Abseil_LIBRARIES "${Abseil_${lib}_LIBRARY}")
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Abseil DEFAULT_MSG Abseil_INCLUDE_DIRS Abseil_LIBRARIES)

foreach(lib ${Abseil_LIBS})
    if (Abseil_FOUND AND NOT TARGET absl::${lib})
        add_library(absl::${lib} INTERFACE IMPORTED)
        set_target_properties(absl::${lib} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Abseil_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${Abseil_${lib}_LIBRARY}"
        )
    endif()
endforeach()

target_link_libraries(absl::strings INTERFACE absl::throw_delegate)
