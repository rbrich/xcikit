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

#ifndef XCI_UTIL_FILE_H
#define XCI_UTIL_FILE_H

#include <string>
#include <functional>

namespace xci {
namespace util {


// Find share dir with fonts and other assets.
// Change working directory to this dir.
void chdir_to_share();

// Try to read whole content of a file.
// Returns empty string in case of any error.
std::string read_file(const std::string& filename);


std::string path_dirname(std::string filename);
std::string path_basename(std::string filename);


std::string get_cwd();


}}  // namespace xci::util

#endif //XCI_UTIL_FILE_H
