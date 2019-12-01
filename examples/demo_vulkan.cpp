// demo_opengl.cpp created on 2018-03-14, part of XCI toolkit
// Copyright 2018, 2019 Radek Brich
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

#include <xci/graphics/vulkan/VulkanRenderer.h>
#include <xci/graphics/vulkan/VulkanWindow.h>
#include <xci/graphics/vulkan/VulkanPrimitives.h>
#include <xci/graphics/vulkan/VulkanShader.h>
#include <xci/core/Vfs.h>
#include <xci/config.h>

#include <cstdlib>

using namespace xci::graphics;
using namespace xci::core;

int main()
{
    Vfs vfs;
    vfs.mount(XCI_SHARE_DIR);

    VulkanRenderer renderer(vfs);
    VulkanWindow window {renderer};
    window.create({800, 600}, "XCI Vulkan Demo");

    VulkanShader shader {renderer.vk_device()};
    shader.load_from_file(
            vfs.read_file("shaders/test_vk.vert.spv").path(),
            vfs.read_file("shaders/test_vk.frag.spv").path());

    VulkanPrimitives primitives {renderer,
        VertexFormat::V2c4t22, PrimitiveType::TriStrips};
    primitives.set_shader(shader);

    window.set_draw_callback([&](View& view) {
        primitives.draw(view);
    });

    window.set_refresh_mode(RefreshMode::Periodic);
    window.display();
    return EXIT_SUCCESS;
}
