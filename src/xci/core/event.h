// event.h created on 2019-03-09 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_EVENT_H
#define XCI_CORE_EVENT_H


#ifdef __APPLE__

#include "event/kqueue/EventLoop.h"
#include "event/kqueue/EventWatch.h"
#include "event/kqueue/FSWatch.h"
#include "event/kqueue/IOWatch.h"
#include "event/kqueue/SignalWatch.h"
#include "event/kqueue/TimerWatch.h"

#elif defined(_WIN32)

#include "event/iocp/EventLoop.h"
#include "event/iocp/EventWatch.h"
#include "event/iocp/FSWatch.h"
#include "event/iocp/IOWatch.h"
#include "event/iocp/SignalWatch.h"
#include "event/iocp/TimerWatch.h"

#elif defined(__linux__) || defined(__EMSCRIPTEN__)

#include "event/epoll/EventLoop.h"
#include "event/epoll/EventWatch.h"
#include "event/epoll/FSWatch.h"
#include "event/epoll/IOWatch.h"
#include "event/epoll/SignalWatch.h"
#include "event/epoll/TimerWatch.h"

#else
#error "No EventLoop implementation."
#endif


#endif // include guard
