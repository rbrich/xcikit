// demo_vfs.cpp created on 2018-09-01, part of XCI toolkit
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

#include <xci/core/Vfs.h>
#include <xci/core/log.h>
#include <xci/config.h>

using namespace xci::core;
using namespace xci::core::log;

int main()
{
    Vfs vfs;
    vfs.mount("/does/not/exist");
    vfs.mount(XCI_SHARE_DIR);

    // This also works:
    //vfs.mount(XCI_SHARE_DIR "/shaders", "shaders");

    // Or create share.dar and mount that:
    //vfs.mount(XCI_SHARE_DIR ".dar");

    auto f = vfs.read_file("non/existent.file");
    log_info("main: open result: {}", f.is_open());

    f = vfs.read_file("shaders/fps.frag");
    log_info("main: open result: {}", f.is_open());
    auto content = f.content();
    if (content)
        log_info("main: file size: {}", content->size());

    // Note that leading slashes in VFS paths don't matter (they are auto-normalized).
    // The VFS paths are always absolute, there is no CWD.
    // Same as above:
    //vfs.mount(XCI_SHARE_DIR "/shaders", "/shaders");
    // Also same as above:
    //vfs.open("/shaders/fps.frag");

    return 0;
}
