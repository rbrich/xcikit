// FileWatch.cpp created on 2018-03-30, part of XCI toolkit
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

#include "FileWatch.h"

#ifdef XCI_FILEWATCH_INOTIFY
    #include <xci/core/filewatch/FileWatchInotify.h>
    #define XCI_FILEWATCH_CLASS FileWatchInotify
#elif defined(XCI_FILEWATCH_KQUEUE)
    #include <xci/core/filewatch/FileWatchKqueue.h>
    #define XCI_FILEWATCH_CLASS FileWatchKqueue
#else
    #include <xci/core/filewatch/FileWatchDummy.h>
    #define XCI_FILEWATCH_CLASS FileWatchDummy
#endif

namespace xci::core {


FileWatch& FileWatch::default_instance()
{
    static XCI_FILEWATCH_CLASS instance;
    return instance;
}


FileWatchPtr FileWatch::create()
{
    return std::make_shared<XCI_FILEWATCH_CLASS>();
}


}  // namespace xci::core
