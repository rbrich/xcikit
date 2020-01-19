// unistd.h created on 2020-01-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_COMPAT_UNISTD_H
#define XCI_COMPAT_UNISTD_H

#ifndef WIN32
#include <unistd.h>
#include <libgen.h>  // dirname, basename
#else

#include <stdlib.h>
#include <io.h>
#include <direct.h>

using ssize_t = long;

#define chdir _chdir
#define isatty _isatty
#define open _open
#define close _close

inline char *getcwd(char *buf, size_t size) {
     return _getcwd(buf, (int)size);
}

inline ssize_t write(int fd, const void *buf, size_t count) {
     return (ssize_t)_write(fd, buf, (unsigned int)count);
}

inline char *dirname(char *path) {
    if (strchr(path, '/') == nullptr && strchr(path, '\\') == nullptr) {
        static char* dot = ".";
        return dot;
    }
    char* last = &path[strlen(path) - 1];
    if (*last == '/' || *last == '\\')
        *last = 0;
    char drive[_MAX_PATH], dir[_MAX_DIR];
    _splitpath(path, drive, dir, nullptr, nullptr);
    last = &path[strlen(drive) + strlen(dir) - 1];
    if ((*last == '/' || *last == '\\') && (last - path > (int)strlen(drive)))
        *last = 0;
    else
        *(last + 1) = 0;
    return path;
}

inline char *basename(char *path) {
    auto len = strlen(path);
    if (!len)
        return path;
    if (path[len-1] == '/' || path[len-1] == '\\')
        path[len-1] = 0;
    char fname[_MAX_FNAME], ext[_MAX_EXT];
    _splitpath(path, nullptr, nullptr, fname, ext);
    strncpy(path, fname, _MAX_FNAME);
    strncat(path, ext, _MAX_EXT);
    return path;
}

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define PATH_MAX _MAX_PATH

#endif

#endif // include guard
