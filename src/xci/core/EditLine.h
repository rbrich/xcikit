// EditLine.h created on 2021-02-26 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_EDITLINE_H
#define XCI_CORE_EDITLINE_H

#include <xci/core/EditBuffer.h>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace xci::core {

namespace fs = std::filesystem;

/// Command line editor, like readline / libedit
class EditLine {
public:

    // TODO
    void open_history_file(const fs::path& path);

    const char* input(std::string_view prompt);

    // These functions allow to redirect input/output to something else
    // than the default stdin/stdout

    void set_input_fd(int fd) { m_input_fd = fd; }
    void set_feed_only() { m_input_fd = -1; }
    void feed_input(std::string_view data);

    using OutputCallback = std::function<void(std::string_view data)>;
    void set_output_callback(OutputCallback cb) { m_output_cb = std::move(cb); }

    using HighlightCallback = std::function<void(std::string_view data, unsigned cursor)>;
    void set_highlight_callback(HighlightCallback cb) { m_highlight_cb = std::move(cb); }

private:
    void write(std::string_view data) { m_output_cb(data); }
    void write_highlight(std::string_view data, unsigned cursor) { m_highlight_cb(data, cursor); }

    /// Obtain more input data from terminal
    /// \returns    false on EOF or error
    bool read_input();

    // editing
    EditBuffer m_edit_buffer;

    // feed input
    int m_input_fd = 0;
    std::string m_input_buffer;
    std::mutex m_input_mutex;
    std::condition_variable m_input_cv;

    // output
    OutputCallback m_output_cb = [](std::string_view data){ std::cout << data << std::flush; };
    HighlightCallback m_highlight_cb = [](std::string_view data, unsigned cursor){ std::cout << data << std::flush; };

    // history
    std::fstream m_history_file;
};

} // namespace xci::core

#endif // include guard
