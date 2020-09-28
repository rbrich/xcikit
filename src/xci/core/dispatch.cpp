// dispatch.cpp created on 2018-03-30, part of XCI toolkit
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

#include "dispatch.h"
#include <xci/core/log.h>

namespace xci::core {


Dispatch::Dispatch()
{
    m_thread = std::thread([this]() {
        log::debug("Dispatch: Thread starting");
        m_loop.run();
        log::debug("Dispatch: Thread finished");
    });
}


Dispatch::~Dispatch()
{
    // Signal the thread to quit
    m_quit_event.fire();
    m_thread.join();
}


bool FSDispatch::add_watch(const std::string& pathname, Callback cb)
{
    return m_fs_watch.add(pathname, std::move(cb));
}


bool FSDispatch::remove_watch(const std::string& pathname)
{
    return m_fs_watch.remove(pathname);
}


}  // namespace xci::core
