// VulkanRenderer.cpp created on 2019-10-23, part of XCI toolkit
// Copyright 2019 Radek Brich
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

#include "VulkanRenderer.h"
#include "VulkanTexture.h"
#include <memory>

namespace xci::graphics {

using std::make_unique;


VulkanRenderer::VulkanRenderer()
{
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "an xci-graphics based app";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "xci-graphics";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    createInfo.enabledLayerCount = 0;

    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create VkInstance!");
    }
}


VulkanRenderer::~VulkanRenderer()
{
    vkDestroyInstance(m_instance, nullptr);
}


Renderer& Renderer::default_instance()
{
    static VulkanRenderer instance;
    return instance;
}


TexturePtr VulkanRenderer::create_texture()
{
    return std::make_shared<VulkanTexture>();
}


ShaderPtr VulkanRenderer::create_shader()
{
    return xci::graphics::ShaderPtr();
}


PrimitivesPtr VulkanRenderer::create_primitives(VertexFormat format,
                                                PrimitiveType type)
{
    return xci::graphics::PrimitivesPtr();
}


} // namespace xci::graphics
