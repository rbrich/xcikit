// Stream.cpp created on 2021-04-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Stream.h"
#include <xci/core/log.h>
#include <xci/core/template/helpers.h>
#include <xci/compat/unistd.h>
#include <xci/compat/macros.h>
#include <fmt/ostream.h>
#include <cassert>

namespace xci::script {

using namespace xci::core;


Stream Stream::raw_stdin() { return Stream(FdRef{STDIN_FILENO}); }
Stream Stream::raw_stdout() { return Stream(FdRef{STDOUT_FILENO}); }
Stream Stream::raw_stderr() { return Stream(FdRef{STDERR_FILENO}); }

#ifdef __EMSCRIPTEN__
// Xterm.js buffers input, so raw unbuffered read() works same way
// as buffered stdio in normal program
Stream Stream::default_stdin() { return raw_stdin(); }
// The output callback is installed in TermCtl, must write through it
Stream Stream::default_stdout() { return term_out(); }
static constexpr auto raw_stdin_name = "stdin";
static constexpr auto term_stdout_name = "stdout";
static constexpr auto c_stdin_name = "fileref:stdin";
static constexpr auto c_stdout_name = "fileref:stdout";
static constexpr auto c_stderr_name = "stderr";
#else
Stream Stream::default_stdin() { return c_stdin(); }
Stream Stream::default_stdout() { return c_stdout(); }
static constexpr auto c_stdin_name = "stdin";
static constexpr auto c_stdout_name = "stdout";
static constexpr auto c_stderr_name = "stderr";
static constexpr auto raw_stdin_name = "fdref:stdin";
static constexpr auto term_stdout_name = "term:stdout";
#endif

static constexpr auto raw_stdout_name = "fdref:stdout";
static constexpr auto raw_stderr_name = "fdref:stderr";
static constexpr auto term_stdin_name = "term:stdin";
static constexpr auto term_stderr_name = "term:stderr";


size_t Stream::write(void* data, size_t size)
{
    return std::visit([data, size](auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Undef>) {
            assert(!"can't write to undefined stream");
            return size_t(0);
        } else if constexpr (std::is_same_v<T, Null>)
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
        } else if constexpr (std::is_same_v<T, TermCtlRef>) {
            v.term->write({(const char*) data, size});
            return size;
        }
    }, m_handle);
}


void Stream::flush()
{
    return std::visit([](auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, CFile> || std::is_same_v<T, CFileRef>) {
            if (fflush(v.file) == EOF)
                log::error("fflush: {m}");
        }
    }, m_handle);
}


std::string Stream::read(size_t n)
{
    return std::visit([n](auto&& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Undef>) {
            assert(!"can't read from undefined stream");
            return std::string{};
        } else if constexpr (std::is_same_v<T, Null>)
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
        } else if constexpr (std::is_same_v<T, TermCtlRef>) {
            return v.term->input();
        }
    }, m_handle);
}


size_t Stream::raw_size() const
{
    return sizeof(uint8_t) + std::visit([](auto&& v) -> size_t {
      using T = std::decay_t<decltype(v)>;
      if constexpr (std::is_same_v<T, Undef> || std::is_same_v<T, Null>)
          return size_t(0);
      else
          return sizeof(T);
    }, m_handle);
}


size_t Stream::raw_read(const byte* buffer)
{
    auto idx = uint8_t(m_handle.index());
    constexpr size_t idx_size = sizeof(uint8_t);
    std::memcpy(&idx, buffer, idx_size);
    buffer += idx_size;
    switch (idx) {
        case 0:
            m_handle = Undef{};
            return idx_size;
        case 1:
            m_handle = Null{};
            return idx_size;
        case 2: {
            std::variant_alternative_t<2, HandleVariant> v;
            std::memcpy(&v, buffer, sizeof(v));
            m_handle = v;
            return idx_size + sizeof(v);
        }
        case 3: {
            std::variant_alternative_t<3, HandleVariant> v;
            std::memcpy(&v, buffer, sizeof(v));
            m_handle = v;
            return idx_size + sizeof(v);
        }
        case 4: {
            std::variant_alternative_t<4, HandleVariant> v;
            std::memcpy(&v, buffer, sizeof(v));
            m_handle = v;
            return idx_size + sizeof(v);
        }
        case 5: {
            std::variant_alternative_t<5, HandleVariant> v;
            std::memcpy(&v, buffer, sizeof(v));
            m_handle = v;
            return idx_size + sizeof(v);
        }
        case 6: {
            std::variant_alternative_t<6, HandleVariant> v;
            std::memcpy(&v, buffer, sizeof(v));
            m_handle = v;
            return idx_size + sizeof(v);
        }
        default:
            static_assert(std::variant_size_v<HandleVariant> == 7);
            XCI_UNREACHABLE;
    }
}


size_t Stream::raw_write(byte* buffer) const
{
    auto idx = uint8_t(m_handle.index());
    constexpr size_t idx_size = sizeof(uint8_t);
    std::memcpy(buffer, &idx, idx_size);
    buffer += idx_size;
    return idx_size + std::visit([buffer](auto&& v) -> size_t {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Undef> || std::is_same_v<T, Null>)
            return 0;
        std::memcpy(buffer, &v, sizeof(v));
        return sizeof(v);
    }, m_handle);
}


std::ostream& operator<<(std::ostream& os, const Stream& v)
{
    std::visit(overloaded {
        [&os](const Stream::Undef&) { os << "undef"; },
        [&os](const Stream::Null&) { os << "null"; },
        [&os](const Stream::CFileRef& v) {
            if (v.file == stdin)
                fmt::print(os, c_stdin_name);
            else if (v.file == stdout)
                fmt::print(os, c_stdout_name);
            else if (v.file == stderr)
                fmt::print(os, c_stderr_name);
            else
                fmt::print(os, "fileref:{:x}", uintptr_t(v.file));
        },
        [&os](const Stream::CFile& v) { fmt::print(os, "file:{:x}", uintptr_t(v.file)); },
        [&os](const Stream::FdRef& v) {
          if (v.fd == STDIN_FILENO)
              fmt::print(os, raw_stdin_name);
          else if (v.fd == STDOUT_FILENO)
              fmt::print(os, raw_stdout_name);
          else if (v.fd == STDERR_FILENO)
              fmt::print(os, raw_stderr_name);
          else
               fmt::print(os, "fdref:{}", v.fd);
        },
        [&os](const Stream::Fd& v) { fmt::print(os, "fd:{}", v.fd); },
        [&os](const Stream::TermCtlRef& v) {
            if (v.term == &TermCtl::stdin_instance())
                fmt::print(os, term_stdin_name);
            else if (v.term == &TermCtl::stdout_instance())
                fmt::print(os, term_stdout_name);
            else if (v.term == &TermCtl::stderr_instance())
                fmt::print(os, term_stderr_name);
            else
                fmt::print(os, "term:{}", uintptr_t(v.term));
        },
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
