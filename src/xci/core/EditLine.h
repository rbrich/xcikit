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
/// * highlighting and completion hints: a user callback can add arbitrary escape sequences
///   or append arbitrary text after end of a line (the original text layout must stay unchanged)
/// * multi-line editing: can be triggered by unclosed brackets or by Alt-Enter
/// * history: managed in memory, new items appended to a file and loaded next time
/// * feed input and receive output programmatically
///   (this allows connecting to a virtual terminal without redirecting FDs nor using PTY,
///   which is useful if you have a graphical terminal widget in the same program)
///
/// * TODO: multi-line - remember original column-in-line in up/down movement
/// * TODO: remove duplicate history lines (consequential)
/// * TODO: minimal output to the terminal, i.e. don't clear and refresh everything on each keypress
///   (this can be achieved by comparing already colorized output to previous one and print just
///   the difference - the tail)
/// * TODO: support transpose-chars (C-t), transpose-words (M-t)
/// * TODO: maybe support also upcase/downcase/capitalize shortcuts
/// * TODO: support Bracketed Paste Mode - don't interpret the pasted text
class EditLine {
public:
    enum Flags : uint8_t {
        Multiline = 0x01,
    };
    explicit EditLine(uint8_t flags = 0) : m_flags(flags) {}

    bool is_multiline() const { return m_flags & Multiline; }

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

    using OutputCallback = std::function<void(std::string_view data, bool flush)>;
    void set_output_callback(OutputCallback cb) { m_output_cb = std::move(cb); }

    /// \return true if ENTER should continue to a new line (unclosed bracket etc.)
    struct HighlightResult { std::string hl_data; bool is_open; };
    using HighlightCallback = std::function<HighlightResult(std::string_view data, unsigned cursor)>;
    void set_highlight_callback(HighlightCallback cb) { m_highlight_cb = std::move(cb); }

private:
    void write(std::string_view data, bool flush = false) { m_output_cb(data, flush); }
    HighlightResult highlight(std::string_view data, unsigned cursor) { return m_highlight_cb(data, cursor); }

    /// Obtain more input data from terminal
    /// \returns    false on EOF or error
    bool read_input();

    bool history_previous();
    bool history_next();

    // editing
    EditBuffer m_edit_buffer;
    int m_cursor_up = 0;  // multi-line: how many lines below the prompt is the cursor
    bool m_edit_continue_nl = false;

    // settings
    const uint8_t m_flags = 0;

    // feed input
    int m_input_fd = 0;
    std::string m_input_buffer;
    std::mutex m_input_mutex;
    std::condition_variable m_input_cv;

    // output
    OutputCallback m_output_cb = [](std::string_view data, bool flush) {
        std::cout << data;
        if (flush)
            std::cout.flush();
    };
    HighlightCallback m_highlight_cb;

    // history
    std::fstream m_history_file;
    std::deque<std::string> m_history;
    int m_history_cursor = -1;
    std::string m_history_orig_buffer;  // saved original buffer before descended into history
};

} // namespace xci::core

#endif // include guard
