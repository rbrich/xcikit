// MagnumShader.cpp created on 2018-10-26, part of XCI toolkit
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

#include "MagnumShader.h"
#include "MagnumTexture.h"
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector4.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <cassert>

namespace xci::graphics {

using namespace Magnum;


bool MagnumShader::load_from_file(const std::string& vertex,
                                  const std::string& fragment)
{
    GL::Shader vert{GL::Version::None, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::None, GL::Shader::Type::Fragment};
    vert.addFile(vertex);
    frag.addFile(fragment);

    if (!GL::Shader::compile({vert, frag}))
        return false;

    attachShaders({vert, frag});
    if (!link())
        return false;

    m_ready = true;
    return true;
}


bool MagnumShader::load_from_memory(const char* vertex_data, int vertex_size,
                                    const char* fragment_data, int fragment_size)
{
    GL::Shader vert{GL::Version::None, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::None, GL::Shader::Type::Fragment};

    vert.addSource({vertex_data, size_t(vertex_size)});
    frag.addSource({fragment_data, size_t(fragment_size)});

    if (!GL::Shader::compile({vert, frag}))
        return false;

    attachShaders({vert, frag});
    if (!link())
        return false;

    m_ready = true;
    return true;
}


void MagnumShader::set_uniform(const char* name, float f)
{
    assert(m_ready);
    auto location = uniformLocation(name);
    setUniform(location, f);
}


void MagnumShader::set_uniform(const char* name,
                               float f1, float f2, float f3, float f4)
{
    assert(m_ready);
    auto location = uniformLocation(name);
    setUniform(location, Vector4(f1, f2, f3, f4));
}


void MagnumShader::set_magnum_uniform(const char* name,
                                      const Magnum::Matrix4& mat)
{
    assert(m_ready);
    auto location = uniformLocation(name);
    setUniform(location, mat);
}


void MagnumShader::set_texture(const char* name, TexturePtr& texture)
{
    auto& t = static_cast<MagnumTexture*>(texture.get())->magnum_texture();
    auto location = uniformLocation(name);
    setUniform(location, 0);
    t.bind(0);
}


} // namespace xci::graphics
