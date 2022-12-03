// demo_plugin.cpp created on 2018-04-19 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/SharedLibrary.h>
#include <xci/core/dispatch.h>
#include <xci/core/log.h>
#include <xci/compat/unistd.h>
#include <functional>
#include <atomic>
#include <csignal>

using namespace xci::core;
using namespace xci::core::log;

#ifdef _WIN32
static const char* filename = "./pluggable.dll";
#else
static const char* filename = "./libpluggable.so";
#endif
std::atomic_bool done {false};
std::atomic_bool reload {false};

static void sigterm(int)
{
    done = true;
}

int main()
{
    // Load library

    SharedLibrary lib;

    info("Load: {}", filename);
    if (!lib.open(filename))
        return EXIT_FAILURE;

    auto fn = reinterpret_cast<const char*(*)()>(lib.resolve("sample_text"));
    if (!fn)
        return EXIT_FAILURE;

    // Setup hot reload

    FSDispatchPtr watch = std::make_shared<FSDispatch>();
    int wd = watch->add_watch(filename, [](FSDispatch::Event ev) {
        if (ev == FSDispatch::Event::Create || ev == FSDispatch::Event::Modify)
            reload = true;
    });
    if (wd == -1)
        return EXIT_FAILURE;

    // Main loop

    signal(SIGTERM, sigterm);
    while (!done) {
        info("sample_text: {}", fn());
        sleep(1);

        if (reload) {
            lib.close();
            lib.open(filename);
            fn = reinterpret_cast<const char*(*)()>(lib.resolve("sample_text"));
            reload = false;
        }
    }

    return EXIT_SUCCESS;
}
