// file.cpp created on 2018-03-29, part of XCI toolkit
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

#include "file.h"
#include "FileWatch.h"
#include <xci/config.h>

#include <fstream>
#include <unistd.h>
#include <libgen.h>
#include <cassert>
#include <sys/param.h>
#include <cstring>

namespace xci {
namespace util {


void chdir_to_share()
{
    chdir(XCI_SHARE_DIR);
}


std::string read_file(const std::string& filename)
{
    std::string content;
    std::ifstream f(filename);
    if (!f)
        return content;  // empty

    f.seekg(0, std::ios::end);
    content.resize((size_t) f.tellg());
    f.seekg(0, std::ios::beg);
    f.read(&content[0], content.size());
    if (!f)
        content.clear();

    return content;
}


std::string path_dirname(std::string filename)
{
    // dirname() may modify the argument, so we take it by value
    // (we also make sure that the internal value is null-terminated)
    assert(filename.c_str() == &filename[0]);
    return ::dirname(&filename[0]);
}


std::string path_basename(std::string filename)
{
    assert(filename.c_str() == &filename[0]);
    return ::basename(&filename[0]);
}


std::string get_cwd()
{
    std::string result(MAXPATHLEN, ' ');
    if (getcwd(&result[0], result.size()) == nullptr) {
        return std::string();
    }
    result.resize(strlen(result.data()));
    return result;
}


}}  // namespace xci::util
