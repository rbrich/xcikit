# ------------ #
# Fetch Fonts  #
# -------------#

# Hack font - download and extract in SOURCE dir
message(STATUS "Assets: Hack font")
set(HACK_VERSION "v3.003")
set(HACK_ARCHIVE "Hack-${HACK_VERSION}-ttf.tar.xz")
file(DOWNLOAD
    https://github.com/source-foundry/Hack/releases/download/${HACK_VERSION}/${HACK_ARCHIVE}
    ${CMAKE_CURRENT_SOURCE_DIR}/${HACK_ARCHIVE}
    EXPECTED_HASH SHA1=7ab9e67f718a1a7f3a7bb4426a4a33b949a911f3
    SHOW_PROGRESS)
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Hack/Hack-Regular.ttf)
    file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${HACK_ARCHIVE}
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/Hack
        VERBOSE)
endif()
