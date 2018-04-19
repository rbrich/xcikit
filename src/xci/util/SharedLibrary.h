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

#ifndef XCI_UTIL_SHAREDLIBRARY_H
#define XCI_UTIL_SHAREDLIBRARY_H

#include <string>

namespace xci {
namespace util {


class SharedLibrary {
public:
    ~SharedLibrary() { close(); }

    bool open(const std::string& filename);
    bool close();

    // Returns symbol address or nullptr if not found
    void* resolve(const std::string& symbol);

private:
    void* m_library = nullptr;
};


}} // namespace xci::util

#endif // XCI_UTIL_SHAREDLIBRARY_H
