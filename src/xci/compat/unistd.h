// unistd.h created on 2020-01-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_COMPAT_UNISTD_H
#define XCI_COMPAT_UNISTD_H

#ifndef WIN32
#include <unistd.h>
#else

#include <io.h>

#define isatty _isatty

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#endif

#endif // include guard
