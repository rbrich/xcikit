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
#include <cstdlib>

namespace xci::core {


std::optional<std::string> read_text_file(const fs::path& pathname)
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


BufferPtr read_binary_file(const fs::path& pathname)
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

    return {new Buffer{content, file_size}, [content](auto){ delete[] content; }};
}


bool write(int fd, std::string s)
{
    size_t written = 0;
    while (written != s.size()) {
        ssize_t r = ::write(fd,
                s.data() + written,
                s.size() - written);
        if (r == -1) {
            if (errno == EINTR)
                continue;
            return false;
        }
        assert(r > 0);
        if (r == 0)
            return false;
        written += r;
    }
    return true;
}


}  // namespace xci::core
