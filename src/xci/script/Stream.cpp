// Stream.cpp created on 2021-04-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Stream.h"
#include <xci/core/log.h>
#include <xci/compat/unistd.h>

namespace xci::script {

using namespace xci::core;


Stream& Stream::null()
{
    static Stream stream;
    return stream;
}


Stream& Stream::c_stdin()
{
    static Stream stream(stdin);
    return stream;
}


Stream& Stream::c_stdout()
{
    static Stream stream(stdout);
    return stream;
}


Stream& Stream::c_stderr()
{
    static Stream stream(stderr);
    return stream;
}


Stream& Stream::raw_stdin()
{
    static Stream stream(STDIN_FILENO);
    return stream;
}


Stream& Stream::raw_stdout()
{
    static Stream stream(STDOUT_FILENO);
    return stream;
}


Stream& Stream::raw_stderr()
{
    static Stream stream(STDERR_FILENO);
    return stream;
}


// variant visitor helper
// see https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


size_t Stream::write(void* data, size_t size)
{
    return std::visit(overloaded {
        [size](std::monostate) { return size; },
        [data, size](FILE* f) { return fwrite(data, 1, size, f); },
        [data, size](int fd) {
            ssize_t r;
            do {
                r = ::write(fd, data, size);
            } while (r == -1 && errno == EINTR);
            if (r == -1) {
                log::error("write({}): {m}", fd);
                return (size_t) 0;
            }
            return (size_t) r;
        },
    }, m_handle);
}


void Stream::flush()
{
    return std::visit([](auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, FILE*>) {
            if (fflush(v) == EOF)
                log::error("fflush: {m}");
        }
    }, m_handle);
}


std::string Stream::read(size_t n)
{
    return std::visit(overloaded {
            [](std::monostate) { return std::string{}; },
            [n](FILE* f) {
                std::string res(n, 0);
                auto r = fread(res.data(), 1, res.size(), f);
                res.resize(r);
                return res;
            },
            [n](int fd) {
                std::string res(n, 0);
                auto r = ::read(fd, res.data(), res.size());
                if (r == -1) {
                    log::error("read({}): {m}", fd);
                    return std::string{};
                }
                res.resize(r);
                return res;
            },
    }, m_handle);
}


} // namespace xci::script
