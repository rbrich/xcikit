// Window.cpp created on 2018-04-13, part of XCI toolkit
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

#include "Window.h"

#include <xci/config.h>

#ifdef XCI_WITH_OPENGL
#include <xci/graphics/opengl/GlWindow.h>
#endif

namespace xci {
namespace graphics {


Window& Window::default_window()
{
#ifdef XCI_WITH_OPENGL
    static GlWindow window;
#else
    #error "No window implementation available"
#endif
    return window;
}


}} // namespace xci::graphics
