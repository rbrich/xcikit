// demo_filewatch.cpp created on 2018-04-02, part of XCI toolkit
// Copyright 2018 Radek Brich
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

#include <xci/util/log.h>
#include <xci/util/FileWatch.h>
#include <atomic>
#include <unistd.h>

using namespace xci::util;

int main(int argc, char** argv)
{
    if (argc != 2) {
        log_info("Usage: {} <file_to_watch>", argv[0]);
        return 1;
    }
    std::string filename = argv[1];

    log_info("Watching: {}", filename);
    FileWatch fw;
    std::atomic_bool done {false};
    int wd = fw.add_watch(filename, [&done] (FileWatch::Event ev) {
        switch (ev) {
            case FileWatch::Event::Modify:
                log_info("File modified");
                break;
            case FileWatch::Event::Delete:
                log_info("File deleted");
                done = true;
                break;
        }
    });

    if (wd == -1)
        return 1;

    while (!done) {
        sleep(1);
    }

    fw.remove_watch(wd);
}
