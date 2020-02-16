// macros.h created on 2018-11-20, part of XCI toolkit
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

#ifndef XCI_COMPAT_MACROS_H
#define XCI_COMPAT_MACROS_H


#if __cplusplus >= 201703L || __has_cpp_attribute(fallthrough)
    #define FALLTHROUGH [[fallthrough]]
#else
    #define FALLTHROUGH
#endif


#ifdef _MSC_VER
    #define UNREACHABLE     __assume(0)
#else
    #define UNREACHABLE     __builtin_unreachable()
#endif


// because [[maybe_unused]] can't be applied to a statement
#define UNUSED      (void)

#endif // include guard
