// FileWatch.h created on 2018-03-30, part of XCI toolkit
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

#ifndef XCI_UTIL_FILEWATCH_H
#define XCI_UTIL_FILEWATCH_H

#include <string>
#include <functional>
#include <thread>
#include <map>

namespace xci {
namespace util {


class FileWatch {
public:
    static FileWatch& default_instance();

    FileWatch();
    ~FileWatch();

    enum class Event {
        Modify,     // File modified
        CloseWrite, // File open for writing was closed
        Delete,     // File deleted
    };

    using Callback = std::function<void(Event)>;

    int add_watch(const std::string& filename, Callback cb);
    void remove_watch(int handle);

private:
    int m_inotify;
    int m_quit_pipe[2];
    std::thread m_thread;
    std::map<int, Callback> m_callback;
};


}}  // namespace xci::util

#endif // XCI_UTIL_FILEWATCH_H
