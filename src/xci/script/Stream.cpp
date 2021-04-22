// Stream.cpp created on 2021-04-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Stream.h"
#include <xci/core/log.h>
#include <xci/compat/unistd.h>
#include <xci/core/template/helpers.h>
#include <fmt/ostream.h>

namespace xci::script {

using namespace xci::core;


Stream Stream::raw_stdin() { return Stream(FdRef{STDIN_FILENO}); }
Stream Stream::raw_stdout() { return Stream(FdRef{STDOUT_FILENO}); }
Stream Stream::raw_stderr() { return Stream(FdRef{STDERR_FILENO}); }


// variant visitor helper
// see https://en.cppreference.com/w/cpp/utility/variant/visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


size_t Stream::write(void* data, size_t size)
{
    return std::visit([data, size](auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, NullStream>)
            return size;
        else if constexpr (std::is_same_v<T, CFile> || std::is_same_v<T, CFileRef>) {
            return fwrite(data, 1, size, v.file);
        } else if constexpr (std::is_same_v<T, Fd> || std::is_same_v<T, FdRef>) {
            ssize_t r;
            do {
                r = ::write(v.fd, data, size);
            } while (r == -1 && errno == EINTR);
            if (r == -1) {
                log::error("write({}): {m}", v.fd);
                return (size_t) 0;
            }
            return (size_t) r;
        }
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
    return std::visit([n](auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, NullStream>)
            return std::string{};
        else if constexpr (std::is_same_v<T, CFile> || std::is_same_v<T, CFileRef>) {
            std::string res(n, 0);
            auto r = fread(res.data(), 1, res.size(), v.file);
            res.resize(r);
            return res;
        } else if constexpr (std::is_same_v<T, Fd> || std::is_same_v<T, FdRef>) {
            std::string res(n, 0);
            auto r = ::read(v.fd, res.data(), res.size());
            if (r == -1) {
                log::error("read({}): {m}", v.fd);
                return std::string{};
            }
            res.resize(r);
            return res;
        }
    }, m_handle);
}


size_t Stream::raw_read(const byte* buffer)
{
    uint8_t idx = uint8_t(m_handle.index());
    std::memcpy(&idx, buffer, sizeof(idx));
    buffer += sizeof(idx);
    switch (idx) {
        case 0:
            m_handle = NullStream{};
            break;
        case 1: {
            CFileRef v;
            std::memcpy(&v, buffer, sizeof(v));
            m_handle = v;
            break;
        }
        case 2: {
            CFile v;
            std::memcpy(&v, buffer, sizeof(v));
            m_handle = v;
            break;
        }
        case 3: {
            FdRef v;
            std::memcpy(&v, buffer, sizeof(v));
            m_handle = v;
            break;
        }
        case 4: {
            Fd v;
            std::memcpy(&v, buffer, sizeof(v));
            m_handle = v;
            break;
        }
    }
    return raw_size();
}


size_t Stream::raw_write(byte* buffer) const
{
    uint8_t idx = uint8_t(m_handle.index());
    std::memcpy(buffer, &idx, sizeof(idx));
    buffer += sizeof(idx);
    std::visit([buffer](auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (!std::is_same_v<T, NullStream>)
            std::memcpy(buffer, &v, sizeof(v));
    }, m_handle);
    return raw_size();
}


std::ostream& operator<<(std::ostream& os, const Stream& v)
{
    std::visit(overloaded {
            [](Stream::NullStream) {},
            [&os](Stream::CFileRef v) { fmt::print(os, "fileref:{:x}", uintptr_t(v.file)); },
            [&os](Stream::CFile v) { fmt::print(os, "file:{:x}", uintptr_t(v.file)); },
            [&os](Stream::FdRef v) { fmt::print(os, "fdref:{}", v.fd); },
            [&os](Stream::Fd v) { fmt::print(os, "fd:{}", v.fd); },
    }, v.m_handle);
    return os;
}


void Stream::close()
{
    std::visit(overloaded {
            [](CFile v) { fclose(v.file); },
            [](Fd v) { ::close(v.fd); },
            [](auto) {},
    }, m_handle);
}


} // namespace xci::script
