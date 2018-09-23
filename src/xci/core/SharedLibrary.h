// SharedLibrary.h created on 2018-04-19, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef XCI_CORE_SHAREDLIBRARY_H
#define XCI_CORE_SHAREDLIBRARY_H

#include <string>

namespace xci {
namespace core {


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


}} // namespace xci::core

#endif // XCI_CORE_SHAREDLIBRARY_H
