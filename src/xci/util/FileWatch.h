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
#include <memory>

namespace xci {
namespace util {


class FileWatch;
using FileWatchPtr = std::unique_ptr<FileWatch>;


// FileWatch may be used for auto-reloading of resource files.

class FileWatch {
public:
    static FileWatch& default_instance();
    static FileWatchPtr create();

    enum class Event {
        Create,     // File was created or moved in
        Delete,     // File was deleted or moved away
        Modify,     // File content was modified
        Attrib,     // File attributes were changed
        Stopped,    // The file is no longer watched (containing directory was deleted or moved)
    };

    using Callback = std::function<void(Event)>;

    // Watch file `filename` for changes and call `cb` when an event occurs.
    // It's possible to add more than one callback for the same `filename`.
    // Note that the callback might be called from another thread.
    // Returns new watch handle on success, -1 on error.
    virtual int add_watch(const std::string& filename, Callback cb) = 0;

    // Remove previously added watch with `handle`. Does nothing for handle -1.
    // In case the same file has multiple callbacks installed, this removes
    // just the one identified by `handle`.
    virtual void remove_watch(int handle) = 0;
};


}}  // namespace xci::util

#endif // XCI_UTIL_FILEWATCH_H
