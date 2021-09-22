// string.h created on 2018-03-23 as part of xcikit project
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

// -----------------------------------------------------------------------------
// Concepts

template <typename T> concept StringIterator =
    std::is_same_v<T, std::string::const_iterator> ||
    std::is_same_v<T, std::string_view::const_iterator> ||
    std::is_same_v<T, const char*>;

template <typename T> concept StringReverseIterator =
    std::is_same_v<T, std::string::const_reverse_iterator> ||
    std::is_same_v<T, std::string_view::const_reverse_iterator>;

// -----------------------------------------------------------------------------


constexpr const char* whitespace_chars = "\t\n\v\f\r ";


/// Remove prefix from string.
/// \param str      String to be checked and modified
/// \param prefix   String to be looked up
/// \return         True if prefix was found and removed
bool remove_prefix(std::string& str, const std::string& prefix);

bool remove_suffix(std::string& str, const std::string& suffix);

/// Replace all occurrences of substring by replacement.
std::string replace_all(std::string_view str, std::string_view substring, std::string_view replacement);

/// Append `indentation` spaces after each newline.
inline std::string indent(std::string_view str, unsigned indentation) {
    return replace_all(str, "\n", "\n" + std::string(indentation, ' '));
}

std::vector<std::string_view> split(std::string_view str, char delim, int maxsplit = -1);
std::vector<std::string_view> rsplit(std::string_view str, char delim, int maxsplit = -1);

// Strip chars from start of a string
template <class T>
void lstrip(std::string &str, T strip_chars) {
    str.erase(0, str.find_first_not_of(strip_chars));
}

template <class T>
void lstrip(std::string_view &str, T strip_chars) {
    str.remove_prefix(std::min(str.find_first_not_of(strip_chars), str.size()));
}

template <class S>
void lstrip(S& str) { return lstrip(str, whitespace_chars); }

template <class T>
[[nodiscard]] std::string
lstripped(const std::string &str, T strip_chars) {
    std::string copy(str);
    lstrip(copy, strip_chars);
    return copy;
}

// Strip chars from end of a string
template <class T>
void rstrip(std::string &str, T strip_chars) {
    str.erase(str.find_last_not_of(strip_chars) + 1);
}

template <class T>
void rstrip(std::string_view &str, T strip_chars) {
    str.remove_suffix(str.size() - std::min(str.find_last_not_of(strip_chars) + 1, str.size()));
}

template <class S>
void rstrip(S& str) { return rstrip(str, whitespace_chars); }

template <class T>
[[nodiscard]] std::string
rstripped(const std::string &str, T strip_chars) {
    std::string copy(str);
    rstrip(copy, strip_chars);
    return copy;
}

// Strip chars from both sides of a string

template <class S, class T>
void strip(S& str, T strip_chars) { lstrip(str, strip_chars); rstrip(str, strip_chars); }

template <class S>
void strip(S& str) { lstrip(str); rstrip(str); }


/// Escape non-printable characters with C escape sequences (eg. '\n')
/// \param str          The string to be escaped. May contains '\0'.
/// \param extended     Enable non-standard extension:
///                     * replace ESC with '\e' instead of '\x1b'
std::string escape(std::string_view str, bool extended = false);

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

// Convert UTF16/32 string to UTF8
std::string to_utf8(std::u16string_view wstr);

#ifdef _WIN32
std::string to_utf8(std::wstring_view wstr);
#endif

// Convert single UTF32 char to UTF8 string. Can't fail.
std::string to_utf8(char32_t codepoint);

const char8_t* utf8_next(const char8_t* utf8);

template <StringIterator I>
I utf8_next(I iter) {
    const char8_t* a = reinterpret_cast<const char8_t*>(&*iter);
    return iter + (utf8_next(a) - a);
}

const char8_t* utf8_prev(const char8_t* utf8);

template <StringReverseIterator RI>
RI utf8_prev(RI riter) {
    const char8_t* a = reinterpret_cast<const char8_t*>(&*riter);
    return riter + (a - utf8_prev(a));
}

template <class S, class SSize = typename S::size_type>
SSize utf8_length(const S& str);

/// Convert character offset to byte offset
size_t utf8_offset(std::string_view str, size_t n_chars);

template <class TIter>
TIter utf8_offset_iter(TIter begin, TIter end, size_t n_chars) {
    while (n_chars != 0 && begin != end) {
        begin = utf8_next(begin);
        --n_chars;
    }
    return begin;
}

std::string_view utf8_substr(std::string_view str, size_t pos, size_t count);

/// Convert a single UTF-8 character to Unicode code point.
/// Only the first UTF-8 character is used, rest of input is ignored.
/// In case of error, log error and return 0.
char32_t utf8_codepoint(const char* utf8);

/// Convert a single UTF-8 character to Unicode code point and return number of bytes used.
/// In case of incomplete UTF-8 sequence (input too short), returns {0, 0}.
/// In case of error, returns {0, -1}.
std::pair<int, char32_t> utf8_codepoint_and_length(std::string_view utf8);

/// Check if there is partial UTF-8 character at the end of string
/// \param str  string to be checked
/// \returns    length of the partial char, 0 if there is none
size_t utf8_partial_end(std::string_view str);

/// An implementation of well-known `wcwidth` function.
/// Returns width of Unicode char in fixed-width font cells. Useful for terminals.
int c32_width(char32_t c);

} // namespace xci::core

#endif // XCI_CORE_STRING_H
