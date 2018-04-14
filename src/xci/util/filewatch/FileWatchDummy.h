// FileWatchDummy.h created on 2018-04-14, part of XCI toolkit
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

#ifndef XCI_UTIL_FILEWATCH_DUMMY_H
#define XCI_UTIL_FILEWATCH_DUMMY_H

#include <xci/util/FileWatch.h>

namespace xci {
namespace util {


class FileWatchDummy: public FileWatch {
public:
    FileWatchDummy();

    int add_watch(const std::string& filename, Callback cb) override;
    void remove_watch(int handle) override {}
};


}} // namespace xci::util

#endif // XCI_UTIL_FILEWATCH_DUMMY_H
