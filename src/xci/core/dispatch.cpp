// dispatch.cpp created on 2018-03-30 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

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


bool FSDispatch::add_watch(const fs::path& pathname, Callback cb)
{
    return m_fs_watch.add(pathname, std::move(cb));
}


bool FSDispatch::remove_watch(const fs::path& pathname)
{
    return m_fs_watch.remove(pathname);
}


}  // namespace xci::core
