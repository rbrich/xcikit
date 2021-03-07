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
#include <ctime>
#include <deque>

namespace xci::core {

namespace fs = std::filesystem;

/// Command line editor, like readline / libedit
///
/// Features:
/// * history: managed in memory, new items appended to a file and loaded next time
class EditLine {
public:

    /// Open the file for appending (add_history writes the item immediately)
    /// and load previous history to memory
    void open_history_file(const fs::path& path);

    /// Add history item to memory, and if the history file is open,
    /// write it also to the file.
    void add_history(std::string_view input);

    /// Show prompt, start line editor and return the input when done
    /// \param prompt   Prompt text, will be shown on the line with editor.
    ///                 May contain escape sequences (esp. color).
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

    bool history_previous();
    bool history_next();

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
    std::deque<std::string> m_history;
    int m_history_cursor = -1;
    std::string m_history_orig_buffer;  // saved original buffer before descended into history
};

} // namespace xci::core

#endif // include guard
