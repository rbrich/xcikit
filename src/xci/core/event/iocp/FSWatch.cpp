// iocp/FSWatch.cpp created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FSWatch.h"
#include <xci/core/file.h>
#include <xci/core/log.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <fcntl.h>
#include <dirent.h>

namespace xci::core {


FSWatch::FSWatch(EventLoop& loop, FSWatch::Callback cb)
    : Watch(loop), m_main_cb(std::move(cb))
{}


FSWatch::~FSWatch()
{

}


bool FSWatch::add(const std::string& pathname, FSWatch::PathCallback cb)
{

    return true;
}


bool FSWatch::remove(const std::string& pathname)
{

    return true;
}


void FSWatch::_notify(const struct kevent& event)
{

}


int FSWatch::register_kevent(const std::string& path, uint32_t fflags, bool no_exist_ok)
{

    return fd;
}


void FSWatch::unregister_kevent(int fd)
{

}


} // namespace xci::core
