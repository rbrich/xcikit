// string.h created on 2018-03-23 belongs to XCI Toolkit
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_STRING_H
#define XCI_CORE_STRING_H

#include <string_view>
#include <string>
#include <vector>

namespace xci::core {


constexpr const char* whitespace_chars = "\t\n\v\f\r ";


/// Check if a string starts with another string
/// \param str      String to be checked
/// \param sub      String to be looked up (length should be shorter or same)
bool starts_with(const std::string& str, const std::string& sub);

std::vector<std::string_view> split(std::string_view str, char delim);

// Strip chars from start of a string
template <typename T>
void lstrip(std::string &str, T strip_chars) {
    str.erase(0, str.find_first_not_of(strip_chars));
}
inline
void lstrip(std::string &str) { return lstrip(str, whitespace_chars); }

// Strip chars from end of a string
template <typename T>
void rstrip(std::string &str, T strip_chars) {
    str.erase(str.find_last_not_of(strip_chars) + 1);
}
inline
void rstrip(std::string &str) { return rstrip(str, whitespace_chars); }

inline void strip(std::string &str) { lstrip(str); rstrip(str); }

// Escape non-printable characters with C escape sequences (eg. '\n')
std::string escape(std::string_view str);

// Unescape (expand) C escape sequences (i.e. "\\n" -> "\n")
// This expects the input is well-formatted:
// - a trailing backslash is just ignored
// - unknown escape sequence (e.g. "\\J") is expanded to literal character ("J")
std::string unescape(std::string_view str);

// Convert string to lower case
std::string to_lower(std::string_view str);

// Convert UTF8 string to UTF32, ie. extract Unicode code points.
// In case of invalid source string, logs error and returns empty string.
std::u32string to_utf32(std::string_view utf8);

// Convert single UTF32 char to UTF8 string. Can't fail.
std::string to_utf8(char32_t codepoint);

const char* utf8_next(const char* pos);

std::string::const_reverse_iterator
utf8_prev(std::string::const_reverse_iterator pos);

size_t utf8_length(std::string_view str);

std::string_view utf8_substr(std::string_view str, size_t pos, size_t count);

// Convert single UTF-8 character to Unicode code point.
// Only the first UTF-8 character is used, rest of input is ignored.
// In case of error, return 0.
char32_t utf8_codepoint(const char* utf8);

/// Check if there is partial UTF-8 character at the end of string
/// \param str  string to be checked
/// \returns    length of the partial char, 0 if there is none
size_t utf8_partial_end(std::string_view str);


} // namespace xci::core

#endif // XCI_CORE_STRING_H
