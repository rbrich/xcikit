// EditLine.h created on 2021-02-26 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_EDITLINE_H
#define XCI_CORE_EDITLINE_H

#include <xci/core/EditBuffer.h>
#include <xci/core/TermCtl.h>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <functional>
#include <ctime>
#include <deque>

namespace xci::core {

namespace fs = std::filesystem;

/// Command line editor, like readline / libedit
///
/// Features:
/// * highlighting and completion hints: a user callback can add arbitrary escape sequences
///   or append arbitrary text at the end (the original text layout must stay unchanged)
/// * multi-line editing: can be triggered by unclosed brackets or by Alt-Enter
/// * history: managed in memory, new items appended to a file and loaded next time
/// * feed input and receive output programmatically
///   (this allows connecting to a virtual terminal without redirecting FDs nor using PTY,
///   which is useful if you have a graphical terminal widget in the same program)
///
/// * TODO: multi-line - remember original column-in-line in up/down movement
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

    // -------------------------------------------------------------------------
    // Blocking input

    /// Show prompt, start line editor and return the input when done
    /// \param prompt   Prompt text, will be shown on the line with editor.
    ///                 May contain escape sequences (esp. color).
    /// \returns (ok, content)   !ok when cancelled (Ctrl-C)
    std::pair<bool, std::string_view> input(std::string_view prompt);

    // -------------------------------------------------------------------------
    // Non-blocking (incremental) input

    /// Show prompt and start incremental input
    void start_input(std::string_view prompt);

    /// Feed input data
    void feed_input(std::string_view data) { m_input_buffer += data; }

    /// Process input and advance input state
    /// \returns false = continue feeding, true = done, call finish_input
    bool advance_input();

    /// Finish editing and return result
    /// \returns (ok, content)   !ok when cancelled (Ctrl-C)
    std::pair<bool, std::string_view> finish_input();

    // -------------------------------------------------------------------------

    /// \return true if ENTER should continue to a new line (unclosed bracket etc.)
    struct HighlightResult { std::string hl_data; bool is_open; };
    using HighlightCallback = std::function<HighlightResult(std::string_view data, unsigned cursor)>;
    void set_highlight_callback(HighlightCallback cb) { m_highlight_cb = std::move(cb); }

private:
    void write(std::string_view data);
    void flush();
    HighlightResult highlight(std::string_view data, unsigned cursor) { return m_highlight_cb(data, cursor); }

    /// Obtain more input data from terminal
    /// \returns    false on EOF or error
    bool read_input();

    void process_input();

    /// \returns    true if consumed and buffer state has changed
    bool process_key(TermCtl::Key key);
    bool process_alt_key(TermCtl::Key key);
    bool process_alt_char(char32_t unicode);
    bool process_ctrl_char(char32_t unicode);

    bool history_previous();
    bool history_next();

    // editing
    EditBuffer m_edit_buffer;
    unsigned m_prompt_len = 0;
    unsigned m_cursor_line = 0;  // multi-line: how many lines below the prompt is the cursor
    bool m_edit_continue_nl = false;

    enum class State: uint8_t {
        NeedMoreInputData,  // read more input into buffer and call again
        Continue,       // call again, buffer not yet empty
        ControlBreak,   // cancelled editing (Ctrl-C)
        Finished,       // finished editing (Enter)
    };
    State m_state = State::NeedMoreInputData;

    // settings
    const uint8_t m_flags = 0;

    // input/output
    std::string m_input_buffer;
    std::string m_output_buffer;
    HighlightCallback m_highlight_cb;

    // history
    std::fstream m_history_file;
    std::deque<std::string> m_history;
    int m_history_cursor = -1;
    std::string m_history_orig_buffer;  // saved original buffer before descended into history
};

} // namespace xci::core

#endif // include guard
