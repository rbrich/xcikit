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


namespace xci::data::metaobject {


template <typename TEnum>
class EnumConstant {
public:
    using value_type = TEnum;

    EnumConstant(const char* name, TEnum value) : m_name(name), m_value(value) {}

    const char* name() const { return m_name; }
    const TEnum value() const { return m_value; }

private:
    const char* m_name;
    TEnum m_value;
};


// Define specialization for each reflected enum.
// The tuple contains EnumConstant instances.
template <typename TEnum, typename std::enable_if_t<std::is_enum<TEnum>::value, int> = 0>
inline auto get_metaobject()
{
    return std::make_tuple();
}


template <typename TEnum, typename std::enable_if_t<std::is_enum<TEnum>::value, int> = 0>
constexpr bool has_metaobject()
{
    return !std::is_same<std::tuple<>, decltype(get_metaobject<TEnum>())>::value;
}


template <typename TEnum>
const char* get_enum_constant_name(TEnum value)
{
    const char* result = "<unknown>";
    meta::detail::for_tuple([&result, value](const EnumConstant<TEnum>& ec) {
            if (ec.value() == value)
                result = ec.name();
        }, get_metaobject<TEnum>());
    return result;
}

} // namespace xci::data::metaobject

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


// Reflection macro for struct/class
#define XCI_METAOBJECT_MEMBER(cls, mbr) member(#mbr, &cls::mbr)
#define XCI_METAOBJECT(cls, ...)                                                \
namespace meta {                                                                \
template <> inline auto registerMembers<cls>() {                                \
    return members(                                                             \
            XCI_FOREACH(XCI_METAOBJECT_MEMBER, cls, XCI_COMMA, __VA_ARGS__)     \
    );                                                                          \
}}

// Reflection macro for enum
#define XCI_METAOBJECT_CONSTANT(enm, cst) EnumConstant<enm>(#cst, enm::cst)
#define XCI_METAOBJECT_FOR_ENUM(enum_type, ...)                                 \
namespace xci::data::metaobject {                                               \
template <> inline auto get_metaobject<enum_type>() {                           \
    return std::make_tuple(                                                     \
    XCI_FOREACH(XCI_METAOBJECT_CONSTANT, enum_type, XCI_COMMA, __VA_ARGS__)     \
    );                                                                          \
}}

#endif // include guard
