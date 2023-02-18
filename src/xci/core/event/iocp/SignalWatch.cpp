// iocp/SignalWatch.cpp created on 2020-01-21 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "SignalWatch.h"
#include <xci/core/log.h>
#include <xci/compat/windows.h>
#include <csignal>
#include <cassert>
#include <stdio.h>

namespace xci::core {


static SignalWatch* g_sigwatch = nullptr;


static void _signal_handler(int sig)
{
    TRACE("signal: {}", sig);
    if (g_sigwatch == nullptr)
        return;
    intptr_t signum = sig;
    g_sigwatch->_loop()._post(*g_sigwatch, reinterpret_cast<LPOVERLAPPED>(signum));
}


static BOOL WINAPI _console_handler(DWORD sig)
{
    TRACE("signal: {}", sig);
    if (sig != CTRL_C_EVENT || g_sigwatch == nullptr)
        return FALSE;  // this handler handles only Ctrl-C
    intptr_t signum = 1000 + sig;
    g_sigwatch->_loop()._post(*g_sigwatch, reinterpret_cast<LPOVERLAPPED>(signum));
    return TRUE;
}


SignalWatch::SignalWatch(EventLoop& loop, std::initializer_list<int> signums,
                         SignalWatch::Callback cb)
    : Watch(loop), m_cb(std::move(cb))
{
    assert(g_sigwatch == nullptr);  // only one instance per thread
    g_sigwatch = this;
    for (int sig: signums) {
        if (sig == SIGINT) {
            // register console handler for Ctrl-C
            if (SetConsoleCtrlHandler(_console_handler, TRUE) == 0) {
                log::error("SetConsoleCtrlHandler: {m:l}");
            }
            continue;
        }

        auto ret = signal(sig, _signal_handler);
        if (ret == SIG_ERR) {
            log::error("signal: {m}");
            continue;
        }
        assert(ret == SIG_DFL);  // previous signal handler
        m_signals.emplace_back(sig);
    }
}


SignalWatch::~SignalWatch()
{
    for (int sig: m_signals) {
        if (sig == SIGINT) {
            auto ret = SetConsoleCtrlHandler(_console_handler, FALSE);
            assert(ret != 0);
            (void) ret;
            continue;
        }
        signal(sig, SIG_DFL);
    }
    g_sigwatch = nullptr;
}


void SignalWatch::_notify(LPOVERLAPPED overlapped)
{
    auto signum = reinterpret_cast<intptr_t>(overlapped);
    if (signum >= 1000) {
        assert(signum - 1000 == CTRL_C_EVENT);
        signum = SIGINT;
    }
    if (m_cb)
        m_cb(int(signum));
}


} // namespace xci::core
