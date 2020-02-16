// iocp/IOWatch.cpp created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "IOWatch.h"
#include <xci/core/log.h>
#include <cassert>

namespace xci::core {


IOWatch::IOWatch(EventLoop& loop, int fd, Flags flags, Callback cb)
    : Watch(loop), m_fd(fd), m_cb(std::move(cb))
{
    // TODO: either translate fd to HANDLE (_get_osfhandle)
    //       or change interface to more generic type
    // TODO: use ReadFile / WriteFile with overlapped
    assert(!"not implemented!");
}


IOWatch::~IOWatch()
{

}


void IOWatch::_notify(LPOVERLAPPED overlapped)
{

}


} // namespace xci::core
