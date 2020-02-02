// error.h - created by Radek Brich on 2018-09-23

#ifndef XCI_CORE_ERROR_H
#define XCI_CORE_ERROR_H

#include <exception>
#include "format.h"
#include "log.h"

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
