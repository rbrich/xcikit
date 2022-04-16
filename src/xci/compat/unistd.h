// unistd.h created on 2020-01-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

///=========================================================================///
///                                                                         ///
/// Minimal Unix compatibility header for WIN32                             ///
///                                                                         ///
/// Program compiled with this header should also disable some warnings:    ///
///                                                                         ///
/// -D_CRT_NONSTDC_NO_WARNINGS (because we can't redeclare all the names,   ///
///                             especially with stuff like `struct stats`   ///
///                             it's almost impossible)                     ///
///                                                                         ///
/// -D_CRT_SECURE_NO_WARNINGS  (don't give me warning when I use `open`,    ///
///                             I know what I'm doing and I don't really    ///
///                             care how secure my program is on this       ///
///                             platform, sorry...)                         ///
///                                                                         ///
/// And also some unwanted features:                                        ///
///                                                                         ///
/// -DNOMINMAX                                                              ///
///                                                                         ///
/// This header doesn't add to macro hell, it uses inline functions         ///
/// whenever possible, even for 1:1 mapping. Because nobody likes to fight  ///
/// lower case macros...                                                    ///
///                                                                         ///
///=========================================================================///

#ifndef XCI_COMPAT_UNISTD_H
#define XCI_COMPAT_UNISTD_H

#include <cstdlib>

#ifndef _WIN32

#include <unistd.h>
#include <libgen.h>   // dirname, basename
#include <strings.h>  // strcasecmp, strncasecmp
#include <climits>    // PATH_MAX

#ifndef _GNU_SOURCE
extern char **environ;
#endif

#else

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <direct.h>
#include <windows.h>
#include <type_traits>

using ssize_t = std::make_signed<size_t>::type;

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifndef PATH_MAX
#define PATH_MAX _MAX_PATH
#endif


inline unsigned int sleep(unsigned int seconds) {
    Sleep(seconds * 1000);
    return 0;
}

inline char *getcwd(char *buf, size_t size) {
     return _getcwd(buf, (int)size);
}

inline ssize_t read(int fd, void *buf, size_t count) {
    return (ssize_t)_read(fd, buf, (unsigned int)count);
}

inline ssize_t write(int fd, const void *buf, size_t count) {
     return (ssize_t)_write(fd, buf, (unsigned int)count);
}

inline int pipe(int pipefd[2]) {
    return _pipe(pipefd, 4096, O_BINARY);
}

inline const char *dirname(char *path) {
    if (strchr(path, '/') == nullptr && strchr(path, '\\') == nullptr) {
        static const char* dot = ".";
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

inline const char *basename(char *path) {
    auto len = strlen(path);
    if (!len)
        return path;
    if (path[len-1] == '/' || path[len-1] == '\\')
        path[--len] = 0;
    char fname[_MAX_FNAME], ext[_MAX_EXT];
    _splitpath(path, nullptr, nullptr, fname, ext);
    return path + (len - strlen(fname) - strlen(ext));
}

inline int mkstemp(char *tmpl) {
    if (_mktemp_s(tmpl, strlen(tmpl) + 1) != 0) {
        return -1;
    }
    return open(tmpl, O_RDWR | O_CREAT | O_EXCL);
}

inline int strcasecmp(const char *s1, const char *s2) {
    return _stricmp(s1, s2);
}

inline int strncasecmp(const char *s1, const char *s2, size_t n) {
    return _strnicmp(s1, s2, n);
}

inline FILE *popen(const char *command, const char *mode) {
    return _popen(command, mode);
}

inline int pclose(FILE *stream) { return _pclose(stream); }

#endif  // _WIN32

#endif // include guard
