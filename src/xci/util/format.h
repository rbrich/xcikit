// format.h created on 2018-03-01, part of XCI toolkit

#ifndef XCI_UTIL_FORMAT_H
#define XCI_UTIL_FORMAT_H

#include <iostream>
#include <sstream>

namespace xci {
namespace util {


// Using variadic templates of C++11, we can implement safe format function,
// which checks argument types and does not require type specifiers in format
// string.
//
// See:
// - https://en.wikipedia.org/wiki/Variadic_template


// Terminates evaluation when there are no more arguments
//
// Note that `fmt` is const char*, not std::string. This has two advantages:
// - We can handle part of it to next recursive iteration
// - It guides the user to pass string literal instead of a variable
inline std::string format(const char *fmt)
{
    std::ostringstream stream;
    while (*fmt) {
        if (*fmt == '{') {
            // "{{" -> "{"
            if (*(fmt + 1) == '{') {
                ++fmt;
            }
            // else: silently continue, leaving the '{' intact
        }
        stream << *fmt++;
    }
    return stream.str();
}

// Recursively evaluates arguments
//
// The format string is the like of Python's format(), see:
// - https://docs.python.org/3/library/string.html#formatspec
//
// The only supported placeholder is simple "{}"
template<typename T, typename ...Args>
inline std::string format(const char *fmt, T value, Args... args)
{
    std::ostringstream stream;
    while (*fmt) {
        if (*fmt == '{') {
            // "{{" -> "{"
            if (*(fmt + 1) == '{') {
                ++fmt;
            }
            // "{}" -> replace
            else if (*(fmt + 1) == '}') {
                stream << value;
                fmt += 2;
                stream << format(fmt, args...);
                return stream.str();
            }
            // else: silently continue, leaving the '{' intact
        }
        stream << *fmt++;
    }
    return stream.str();
}


}}  // namespace xci::util

#endif // XCI_UTIL_FORMAT_H
