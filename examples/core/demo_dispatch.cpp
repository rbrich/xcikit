// demo_event.cpp created on 2020-02-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include <xci/core/dispatch.h>
#include <xci/core/log.h>
#include <xci/core/chrono.h>
#include <xci/core/sys.h>
#include <xci/compat/unistd.h>

#include <string>
#include <fstream>

using namespace xci::core;
using namespace xci::core::log;
using std::this_thread::sleep_for;
using namespace std::string_literals;


int main()
{
    Logger::init();
    FSDispatch fw;

    std::string tmpname = fs::temp_directory_path() / "xci_test_filewatch.XXXXXX";
    // race condition not important - would use mktemp, but that causes a warning with glibc
    close(mkstemp(&tmpname[0]));
    std::ofstream f(tmpname);

    fw.add_watch(tmpname, [] (FSDispatch::Event ev) {
        info("Event received: {}", (int) ev);
    });

    info("modify (one)");
    f << "one" << std::endl;
    f.flush();
    sleep_for(100ms);

    info("modify (two)");
    f << "two" << std::endl;
    sleep_for(100ms);

    info("close");
    f.close();
    sleep_for(100ms);

    info("reopen, modify (three), close");
    f.open(tmpname, std::ios::app);
    f << "three" << std::endl;
    f.close();
    sleep_for(100ms);

    // delete
    info("delete");
    ::unlink(tmpname.c_str());
    sleep_for(100ms);

    // although the inotify watch is removed automatically after delete,
    // this should still be called to cleanup the callback info
    fw.remove_watch(tmpname);

    return 0;
}
