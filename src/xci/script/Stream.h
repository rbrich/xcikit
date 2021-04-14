// Stream.h created on 2021-04-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_STREAM_H
#define XCI_SCRIPT_STREAM_H

#include <string>
#include <variant>
#include <span>
#include <cstdio>

namespace xci::script {


template<typename T>
concept ByteSpanT = requires(const T& v) {
    v.data();
    v.size();
};


class Stream {
public:
    Stream() = default;  // null stream
    Stream(FILE* f) : m_handle(f) {}
    Stream(int fd) : m_handle(fd) {}

    static Stream& null();

    // FILE*
    static Stream& c_stdin();
    static Stream& c_stdout();
    static Stream& c_stderr();

    // fd
    static Stream& raw_stdin();
    static Stream& raw_stdout();
    static Stream& raw_stderr();

    template <ByteSpanT T> size_t write(T data) { return write((void*) data.data(), data.size()); }
    size_t write(void* data, size_t size);
    void flush();

    std::string read(size_t n);

private:
    std::variant<
        std::monostate,  // null stream
        FILE*,   // stdio
        int      // FD / socket
    > m_handle;
};


} // namespace xci::script

#endif // include guard
