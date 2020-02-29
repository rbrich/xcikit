// Program.cpp.cc created on 2021-03-20 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Program.h"
#include "Highlighter.h"
#include <xci/script/Error.h>
#include <xci/core/log.h>
#include <xci/core/string.h>
#include <xci/core/file.h>
#include <xci/core/sys.h>
#include <xci/config.h>

namespace xci::script::tool {

using namespace xci::core;

static constexpr const char8_t* intro = u8"{t:bold}{fg:magenta}🔥 fire script{t:normal} {fg:magenta}v{}{t:normal}\n"
                                        "Type {t:bold}{fg:yellow}.h{t:normal} for help, {t:bold}{fg:yellow}.q{t:normal} to quit.\n";
static constexpr const char* prompt = "{fg:green}_{} ?{t:normal} ";


Program::Program(bool log_debug)
{
    Logger::init(log_debug ? Logger::Level::Trace : Logger::Level::Warning);
    vfs.mount(XCI_SHARE);
}


void Program::process_args(char* argv[])
{
    opts.parse(argv);

    if (opts.prog_opts.expr) {
        if (!repl.evaluate("<input>", opts.prog_opts.expr, EvalMode::SingleInput))
            exit(1);
        exit(0);
    }

    if (!opts.prog_opts.input_files.empty()) {
        for (const char* input_file : opts.prog_opts.input_files) {
            auto content = read_text_file(input_file);
            if (!content) {
                std::cerr << "cannot read file: " << input_file << std::endl;
                exit(1);
            }
            auto module_name = fs::path(input_file).filename().replace_extension().string();
            if (!repl.evaluate(module_name, *content,
                    opts.prog_opts.compile ? EvalMode::Compile : EvalMode::SingleInput))
                exit(1);
            if (opts.prog_opts.compile) {
                std::string out_path = opts.prog_opts.output_file;
                if (out_path.empty()) {
                    out_path = input_file;
                    if (out_path.ends_with(".fire"))
                        out_path.back() = 'm';
                    else
                        out_path += ".firm";
                }
                if (!ctx.input_modules.back()->save(out_path))
                    exit(1);
            }
        }
        exit(0);
    }

    if (opts.prog_opts.compile) {
        std::cerr << "--compile: no input files" << std::endl;
        exit(1);
    }
}


void Program::repl_init()
{
    TermCtl& t = ctx.term_out;
    auto history_file = xci::core::home_directory_path() / ".xci_fire_history";
    edit_line().open_history_file(history_file);
    edit_line().set_highlight_callback([&t](std::string_view data, unsigned cursor) {
        auto [hl_data, is_open] = Highlighter(t).highlight(data, cursor);
        return EditLine::HighlightResult{hl_data, is_open};
    });

    t.print((const char*)intro, XCI_VERSION);
    ctx.input_number = 0;
}


void Program::repl_loop()
{
    TermCtl& t = ctx.term_out;
    bool done = false;
    repl_command().set_quit_cb([&done]{ done = true; });
    while (!done) {
        auto [ok, line] = edit_line().input(t.format(prompt, ctx.input_number));
        if (!ok) {
            ctx.term_out.write("\n");
            break;
        }
        evaluate_input(line);
    }
}


void Program::repl_prompt()
{
    edit_line().start_input(ctx.term_out.format(prompt, ctx.input_number));
}


void Program::repl_step(std::string_view partial_input)
{
    if (! edit_line().feed_input(partial_input)) {
        auto [ok, line] = edit_line().finish_input();
        if (ok)
            evaluate_input(line);
        // previous input is finished, start a new one
        repl_prompt();
    }
}


void Program::evaluate_input(std::string_view input)
{
    strip(input);
    if (input.empty())
        return;

    edit_line().add_history(input);

    if (input[0] == '.') {
        // control commands
        try {
            repl_command().eval(input.substr(1));
        } catch (const ScriptError& e) {
            auto& tout = ctx.term_out;
            tout.print("{fg:red}Error: {t:bold}{}{t:normal}\n", e.what());
            if (!e.detail().empty())
                tout.print("{fg:magenta}{}{t:normal}\n", e.detail());
            tout.print("{fg:yellow}Help: .h | .help{t:normal}\n");
        }
        return;
    }

    if (repl.evaluate(fmt::format("_{}", ctx.input_number), std::string(input), EvalMode::Repl))
        ++ ctx.input_number;
}


ReplCommand& Program::repl_command()
{
    static ReplCommand cmd(ctx);
    return cmd;
}


core::EditLine& Program::edit_line()
{
    static core::EditLine edit_line{core::EditLine::Multiline};
    return edit_line;
}


} // namespace xci::script::tool
