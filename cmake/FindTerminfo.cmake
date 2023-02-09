# Try to find terminfo or TInfo (ncurses) library
#
# Once done this will define:
#   Terminfo_FOUND
#   Terminfo_LIBRARIES
#   Terminfo::terminfo (imported library target)

find_library(Terminfo_LIBRARY NAMES terminfo tinfo curses ncurses ncursesw)
mark_as_advanced(Terminfo_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Terminfo DEFAULT_MSG Terminfo_LIBRARY)

if (Terminfo_FOUND)
    set(Terminfo_LIBRARIES ${Terminfo_LIBRARY})
    if (NOT TARGET Terminfo::terminfo)
        add_library(Terminfo::terminfo INTERFACE IMPORTED)
        set_target_properties(Terminfo::terminfo PROPERTIES
            INTERFACE_LINK_LIBRARIES "${Terminfo_LIBRARY}")
    endif()
endif()
