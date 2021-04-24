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

using std::byte;


template<typename T>
concept ByteSpanT = requires(const T& v) {
    v.data();
    v.size();
};


class Stream {
public:
    // null stream (/dev/null)
    using NullStream = std::monostate;

    // stdio
    struct CFileRef {
        // not owned, won't be closed
        FILE* file;
        bool operator ==(const CFileRef& rhs) const = default;
    };
    struct CFile {
        // owned, will be closed
        FILE* file;
        bool operator ==(const CFile& rhs) const = default;
    };
    static_assert(sizeof(CFile) == sizeof(void*));

    // FD / socket
    struct FdRef {
        // not owned, won't be closed
        int fd;
        bool operator ==(const FdRef& rhs) const = default;
    };
    struct Fd {
        // owned, will be closed
        int fd;
        bool operator ==(const Fd& rhs) const = default;
    };
    static_assert(sizeof(Fd) == sizeof(int));

    Stream() = default;  // null stream

    template <class T>
    explicit Stream(T&& v) : m_handle(std::forward<T>(v)) {}

    bool operator ==(const Stream& rhs) const = default;
    friend std::ostream& operator<<(std::ostream& os, const Stream& v);

    static Stream null() { return Stream(NullStream{}); }

    // FILE*
    static Stream c_stdin() { return Stream(CFileRef{stdin}); }
    static Stream c_stdout() { return Stream(CFileRef{stdout}); }
    static Stream c_stderr() { return Stream(CFileRef{stderr}); }

    // fd
    static Stream raw_stdin();
    static Stream raw_stdout();
    static Stream raw_stderr();

    template <ByteSpanT T> size_t write(T data) { return write((void*) data.data(), data.size()); }
    size_t write(void* data, size_t size);
    void flush();

    std::string read(size_t n);

    // heap serialization
    constexpr static size_t raw_size() { return 1 + sizeof m_handle; }
    size_t raw_read(const byte* buffer);
    size_t raw_write(byte* buffer) const;

    void close();

private:
    std::variant<NullStream, CFileRef, CFile, FdRef, Fd> m_handle;
};


// Stream is just two pointers big, same as string_view. Safe to pass by value.
static_assert(sizeof(Stream) == 2 * sizeof(void*));


} // namespace xci::script

#endif // include guard
