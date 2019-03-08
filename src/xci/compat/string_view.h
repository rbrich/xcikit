// string_view.h created on 2019-03-08, part of XCI toolkit
// Copyright 2019 Radek Brich
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

#ifndef XCI_COMPAT_STRING_VIEW_H
#define XCI_COMPAT_STRING_VIEW_H

#if __cpp_lib_string_view >= 201606L

#include <string_view>
namespace xci {
    using std::string_view;
}

#else

#include <experimental/string_view>
namespace xci {
    using std::experimental::string_view;
}

#endif

#endif // include guard
