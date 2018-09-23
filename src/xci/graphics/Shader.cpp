// Shader.cpp created on 2018-09-02, part of XCI toolkit
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

#include "Shader.h"
#include <xci/core/Vfs.h>
#include <xci/core/file.h>


namespace xci::graphics {

using namespace xci::core;


bool Shader::load_from_vfs(const std::string& vertex, const std::string& fragment)
{
    auto& vfs = Vfs::default_instance();
    auto vert_file = vfs.open(vertex);
    auto frag_file = vfs.open(fragment);
    if (vert_file.is_real_file() && frag_file.is_real_file()) {
        return load_from_file(vert_file.path(), frag_file.path());
    } else {
        auto vert_data = read_text_file(vert_file);
        auto frag_data = read_text_file(frag_file);
        return load_from_memory(
                vert_data.data(), (int) vert_data.size(),
                frag_data.data(), (int) frag_data.size());
    }
}


} // namespace xci::graphics
