// event.h created on 2019-03-09, part of XCI toolkit
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

#elif defined(__linux__)

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
