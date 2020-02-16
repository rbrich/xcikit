// SharedLibrary.h created on 2018-04-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_SHARED_LIBRARY_H
#define XCI_CORE_SHARED_LIBRARY_H

#include <string>

namespace xci::core {


// Dynamically load shared object files
//
// Inspiration:
// - http://doc.qt.io/qt-5/qlibrary.html
// - https://developer.gnome.org/glib/stable/glib-Dynamic-Loading-of-Modules.html

class SharedLibrary {
public:
    ~SharedLibrary() { close(); }

    // Load library with `filename` or initialize this instance
    // with previously loaded library (references are counted)
    bool open(const std::string& filename);

    // Unload the library if this was the last instance referencing it.
    bool close();

    // Returns symbol address or nullptr if not found
    void* resolve(const std::string& symbol);

private:
    void* m_library = nullptr;
};


} // namespace xci::core

#endif // include guard
