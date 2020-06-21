// foreach.h created on 2020-06-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_MACROS_FOREACH_H
#define XCI_CORE_MACROS_FOREACH_H


#define XCI_COMMA() ,
#define XCI_GET_NTH_(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,fn,...) fn
#define XCI_FOR_1(f,arg,sep,x)     f(arg,x)
#define XCI_FOR_2(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_1(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_3(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_2(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_4(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_3(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_5(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_4(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_6(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_5(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_7(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_6(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_8(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_7(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_9(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_8(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_10(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_9(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_11(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_10(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_12(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_11(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_13(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_12(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_14(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_13(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_15(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_14(f,arg,sep,__VA_ARGS__)
#define XCI_FOR_16(f,arg,sep,x,...) f(arg,x)sep()XCI_FOR_15(f,arg,sep,__VA_ARGS__)
#define XCI_FOREACH(f, arg, sep, ...)                                                               \
    XCI_GET_NTH_(__VA_ARGS__,XCI_FOR_16,XCI_FOR_15,XCI_FOR_14,XCI_FOR_13,XCI_FOR_12,XCI_FOR_11,     \
        XCI_FOR_10,XCI_FOR_9,XCI_FOR_8,XCI_FOR_7,XCI_FOR_6,XCI_FOR_5,XCI_FOR_4,XCI_FOR_3,XCI_FOR_2, \
        XCI_FOR_1)(f,arg,sep,__VA_ARGS__)


#endif // include guard
