// demo_filewatch.cpp created on 2018-04-02 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/log.h>
#include <xci/core/event.h>
#include <csignal>
#include <cstdlib>

using namespace xci::core;
using namespace xci::core::log;


int main(int argc, char** argv)
{
    if (argc != 2) {
        info("Usage: {} <file_to_watch>", argv[0]);
        return 1;
    }
    std::string filename = argv[1];

    info("Demo: Watching {}", filename);
    EventLoop loop;
    FSWatch fs_watch(loop);
    bool ok = fs_watch.add(filename, [&loop](FSWatch::Event ev) {
        switch (ev) {
            case FSWatch::Event::Create:
                info("File created / moved in");
                break;
            case FSWatch::Event::Delete:
                info("File deleted / moved away");
                break;
            case FSWatch::Event::Modify:
                info("File modified");
                break;
            case FSWatch::Event::Attrib:
                info("File touched (attribs changed)");
                break;
            case FSWatch::Event::Stopped:
                info("File watching stopped (dir deleted / moved)");
                loop.terminate();
                break;
        }
    });
    if (!ok)
        return EXIT_FAILURE;

    SignalWatch signal_watch(loop, {SIGTERM}, [&loop](int signum) {
        (void) signum;
        loop.terminate();
    });

    loop.run();
    return EXIT_SUCCESS;
}
