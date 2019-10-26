// DefaultWindow.h created on 2019-10-26, part of XCI toolkit
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

#ifndef XCI_GRAPHICS_DEFAULT_WINDOW_H
#define XCI_GRAPHICS_DEFAULT_WINDOW_H

#include <xci/config.h>


#ifdef XCI_WITH_VULKAN
#include "vulkan/VulkanWindow.h"
    namespace xci::graphics {
        using DefaultWindow = VulkanWindow;
    }
#elif defined(XCI_WITH_OPENGL)
    #include "opengl/GlWindow.h"
    namespace xci::graphics {
    using DefaultWindow = GlWindow;
    }
#else
#include "Window.h"
    namespace xci::graphics {
        using DefaultWindow = Window;  // this will cause error on use
    }
#endif


#endif // include guard
