// file.h created on 2018-03-29, part of XCI toolkit
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

#ifndef XCI_CORE_FILE_H
#define XCI_CORE_FILE_H

#include "Buffer.h"
#include <string>
#include <vector>
#include <functional>
#include <optional>

namespace xci::core {


// Try to read whole content of a file.
// Returns empty string in case of any error.
std::optional<std::string> read_text_file(const std::string& pathname);
std::optional<std::string> read_text_file(std::istream& stream);
BufferPtr read_binary_file(const std::string& pathname);
BufferPtr read_binary_file(std::istream& stream);

/// Write string to FD (in a loop, handling EINTR).
/// \returns false on error (check errno), true on success
bool write(int fd, std::string s);

namespace path {

// C++ wrappers for well known Unix functions
// (the names are intentionally adjusted a little to avoid collision
// with possible macros, e.g. `basename` on Linux)

std::string dir_name(std::string pathname);
std::string base_name(std::string pathname);
std::string join(const std::string &part1, const std::string &part2);
std::string real_path(const std::string& path);
std::string cwd();

} // namespace path


}  // namespace xci::core

#endif //XCI_CORE_FILE_H
