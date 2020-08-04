// demo_filewatch.cpp created on 2018-04-02, part of XCI toolkit
// Copyright 2018,2019 Radek Brich
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

#include <xci/core/log.h>
#include <xci/core/event.h>
#include <csignal>
#include <cstdlib>

using namespace xci::core;


int main(int argc, char** argv)
{
    if (argc != 2) {
        log_info("Usage: {} <file_to_watch>", argv[0]);
        return 1;
    }
    std::string filename = argv[1];

    log_info("Demo: Watching {}", filename);
    EventLoop loop;
    FSWatch fs_watch(loop);
    bool ok = fs_watch.add(filename, [&loop](FSWatch::Event ev) {
        switch (ev) {
            case FSWatch::Event::Create:
                log_info("File created / moved in");
                break;
            case FSWatch::Event::Delete:
                log_info("File deleted / moved away");
                break;
            case FSWatch::Event::Modify:
                log_info("File modified");
                break;
            case FSWatch::Event::Attrib:
                log_info("File touched (attribs changed)");
                break;
            case FSWatch::Event::Stopped:
                log_info("File watching stopped (dir deleted / moved)");
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
