// error.h created on 2018-09-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_ERROR_H
#define XCI_CORE_ERROR_H

#include <exception>
#include <string>

namespace xci::core {


class Error: public std::exception {
public:
    explicit Error(std::string msg) : m_msg(std::move(msg)) {}

    const char* what() const noexcept override {
        return m_msg.c_str();
    }

private:
    std::string m_msg;
};


} // namespace xci::core

#endif // XCI_CORE_ERROR_H
