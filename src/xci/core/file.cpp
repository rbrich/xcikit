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
#include <xci/compat/unistd.h>
#include <xci/config.h>

#include <fstream>
#include <cassert>
#include <climits>
#include <cstring>

namespace xci::core {


std::optional<std::string> read_text_file(const std::string& pathname)
{
    std::ifstream f(pathname);
    return read_text_file(f);
}


std::optional<std::string> read_text_file(std::istream& stream)
{
    if (!stream)
        return {};

    stream.seekg(0, std::ios::end);
    auto file_size = size_t(stream.tellg());
    stream.seekg(0, std::ios::beg);
    std::string content(file_size, char(0));
    stream.read(&content[0], content.size());
    if (!stream)
        content.clear();

    return content;
}


BufferPtr read_binary_file(const std::string& pathname)
{
    std::ifstream f(pathname, std::ios::binary);
    return read_binary_file(f);
}


BufferPtr read_binary_file(std::istream& stream)
{
    if (!stream)
        return {};

    stream.seekg(0, std::ios::end);
    auto file_size = size_t(stream.tellg());
    stream.seekg(0, std::ios::beg);

    auto* content = new byte[file_size];
    stream.read(reinterpret_cast<char*>(content), file_size);
    if (!stream) {
        delete[] content;
        return {};
    }

    return {new Buffer{content, file_size}, [content](auto p){ delete[] content; }};
}


std::string path_dirname(std::string pathname)
{
    // dirname() may modify the argument, so we take it by value
    // (we also make sure that the internal value is null-terminated)
    assert(pathname.c_str() == &pathname[0]);
    return ::dirname(&pathname[0]);
}


std::string path_basename(std::string pathname)
{
    assert(pathname.c_str() == &pathname[0]);
    return ::basename(&pathname[0]);
}


std::string path_join(const std::string &part1, const std::string &part2)
{
    // Add separator between parts only if there is none already
    std::string sep = "/";
    if (!part1.empty() && part1[part1.size() - 1] == '/')
        sep.clear();
    if (!part2.empty() && part2[0] == '/')
        sep.clear();
    return part1 + sep + part2;
}


std::string get_cwd()
{
    std::string result(PATH_MAX, ' ');
    if (getcwd(&result[0], result.size()) == nullptr) {
        return std::string();
    }
    result.resize(strlen(result.data()));
    return result;
}


}  // namespace xci::core
