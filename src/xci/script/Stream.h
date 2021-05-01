// Stream.h created on 2021-04-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_STREAM_H
#define XCI_SCRIPT_STREAM_H

#include <xci/core/TermCtl.h>
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
    // undefined - no value
    using Undef = std::monostate;

    // null stream (/dev/null)
    struct Null {
        bool operator ==(const Null& rhs) const { return true; }
    };

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

    // TermCtl
    struct TermCtlRef {
        // not owned
        core::TermCtl* term;
        bool operator ==(const TermCtlRef& rhs) const = default;
    };

    using HandleVariant = std::variant<Undef, Null, CFileRef, CFile, FdRef, Fd, TermCtlRef>;

    Stream() = default;  // undef stream

    template <class T>
    requires requires (HandleVariant x, T y) { x = y; }
    explicit Stream(T&& v) : m_handle(std::forward<T>(v)) {}

    explicit operator bool() const { return !std::holds_alternative<Undef>(m_handle); }
    bool operator ==(const Stream& rhs) const = default;
    friend std::ostream& operator<<(std::ostream& os, const Stream& v);

    static Stream null() { return Stream(Null{}); }

    // FILE*
    static Stream c_stdin() { return Stream(CFileRef{stdin}); }
    static Stream c_stdout() { return Stream(CFileRef{stdout}); }
    static Stream c_stderr() { return Stream(CFileRef{stderr}); }

    // fd
    static Stream raw_stdin();
    static Stream raw_stdout();
    static Stream raw_stderr();

    // TermCtl
    static Stream term_in() { return Stream(TermCtlRef{&core::TermCtl::stdin_instance()}); }
    static Stream term_out() { return Stream(TermCtlRef{&core::TermCtl::stdout_instance()}); }
    static Stream term_err() { return Stream(TermCtlRef{&core::TermCtl::stderr_instance()}); }

    // sane default
    static Stream default_stdin();
    static Stream default_stdout();
    static Stream default_stderr() { return c_stderr(); }

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
    HandleVariant m_handle;
};


// Stream is just two pointers big, same as string_view. Safe to pass by value.
static_assert(sizeof(Stream) == 2 * sizeof(void*));


} // namespace xci::script

#endif // include guard
