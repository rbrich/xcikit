// reflection.h created on 2019-03-13, part of XCI toolkit
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

#ifndef XCI_DATA_REFLECTION_H
#define XCI_DATA_REFLECTION_H

#include <meta/Meta.h>


// Macro utility
#define XCI_COMMA() ,
#define XCI_GET_NTH_(a1,a2,a3,a4,a5,a6,a7,a8,a9,fn,...) fn
#define XCI_FOR_1(f,arg,sep,x)     f(arg,x)
#define XCI_FOR_2(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_1(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_3(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_2(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_4(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_3(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_5(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_4(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_6(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_5(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_7(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_6(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_8(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_7(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_9(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_8(f,arg,sep,__VA_ARGS__)
#define XCI_FOREACH(f, arg, sep, ...)                                           \
    XCI_GET_NTH_(__VA_ARGS__,XCI_FOR_9,XCI_FOR_8,XCI_FOR_7,XCI_FOR_6,XCI_FOR_5, \
    XCI_FOR_4,XCI_FOR_3,XCI_FOR_2,XCI_FOR_1)(f,arg,sep,__VA_ARGS__)


// Reflection macro
#define XCI_DATA_MEMBER(cls, mbr) member(#mbr, &cls::mbr)
#define XCI_DATA_REFLECT(cls, ...)                                              \
namespace meta {                                                                \
template <> inline auto registerMembers<cls>() {                                \
    return members(                                                             \
            XCI_FOREACH(XCI_DATA_MEMBER, cls, XCI_COMMA, __VA_ARGS__)           \
    );                                                                          \
}}

#endif // include guard
