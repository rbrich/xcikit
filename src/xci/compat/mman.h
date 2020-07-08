// mmap.h created on 2020-01-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_COMPAT_MMAP_H
#define XCI_COMPAT_MMAP_H

#ifndef _WIN32
    #include <sys/mman.h>
#else
    #include <mman-win32/mman.h>  // see external/mman-win32
#endif

#endif // include guard
