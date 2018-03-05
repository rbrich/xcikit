# Find Panda3D
#
# optionally set PANDA3D_ROOT to custom root

set(PANDA3D_SEARCH_PATHS
    ${PANDA3D_ROOT}
    /Developer/Panda3D)

find_path(PANDA3D_INCLUDE_DIR
    NAMES pandaFramework.h
    PATH_SUFFIXES include panda3d
    PATHS ${PANDA3D_SEARCH_PATHS})

set(PANDA3D_ALL_LIBRARIES "")
set(PANDA3D_ALL_LIBRARY_NAMES "")
foreach (libname p3framework panda pandaexpress p3dtoolconfig p3dtool p3pystub p3direct)
    string(TOUPPER ${libname} ulibname)
    find_library(PANDA3D_${ulibname}_LIBRARY
        NAMES ${libname}
        PATH_SUFFIXES lib panda3d
        PATHS ${PANDA3D_SEARCH_PATHS})
    set(PANDA3D_ALL_LIBRARY_NAMES ${PANDA3D_ALL_LIBRARY_NAMES} PANDA3D_${ulibname}_LIBRARY)
    set(PANDA3D_ALL_LIBRARIES ${PANDA3D_ALL_LIBRARIES} ${PANDA3D_${ulibname}_LIBRARY})
endforeach (libname)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Panda3D DEFAULT_MSG
    PANDA3D_INCLUDE_DIR ${PANDA3D_ALL_LIBRARY_NAMES})

set(PANDA3D_INCLUDE_DIRS ${PANDA3D_INCLUDE_DIR})
set(PANDA3D_LIBRARIES ${PANDA3D_ALL_LIBRARIES})

#message("PANDA3D_INCLUDE_DIRS: ${PANDA3D_INCLUDE_DIRS}")
#message("PANDA3D_LIBRARIES: ${PANDA3D_LIBRARIES}")
