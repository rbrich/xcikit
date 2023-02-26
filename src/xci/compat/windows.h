// windows.h created on 2023-02-18 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_COMPAT_WINDOWS_H
#define XCI_COMPAT_WINDOWS_H

/// This header includes minimal Windows.h for types like DWORD, HANDLE
/// If we ever needed something from Windows on other platforms (not very probable),
/// we could also define such polyfills here.

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NOIME
#define NOGDI
#define NOMCX
#define NOSERVICE
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#undef NOIME
#undef NOGDI
#undef NOMCX
#undef NOSERVICE

#endif  // _WIN32

#endif // include guard
