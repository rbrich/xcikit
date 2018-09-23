// error.h - created by Radek Brich on 2018-09-23

#ifndef XCI_UTIL_ERROR_H
#define XCI_UTIL_ERROR_H

#include <exception>
#include "format.h"
#include "log.h"

namespace xci::util {


class Error: public std::exception {
public:
    explicit Error(std::string detail) : m_detail(std::move(detail)) {}

    const char* what() const noexcept override {
        return m_detail.c_str();
    }

private:
    std::string m_detail;
};


} // namespace xci::util

#endif // XCI_UTIL_ERROR_H
