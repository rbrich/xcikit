// main.cpp created on 2019-05-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019, 2020, 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Program.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#endif

using namespace xci::script::tool;


#ifdef __EMSCRIPTEN__

using namespace emscripten;

// Translate JS callback function to C++
static void prog_set_term_output_cb(Program& prog, val write_cb) {
    if (write_cb.isNull())
        return;
    prog.ctx.term_out.set_write_callback([write_cb](std::string_view sv) {
        std::string str{sv};
        write_cb(str);
    });
}

static void prog_set_quit_cb(Program& prog, val quit_cb) {
    if (quit_cb.isNull())
        return;
    prog.repl_command().set_quit_cb([quit_cb] {
        quit_cb();
    });
}

// Translate string to string_view (embind doesn't know string_view)
static void prog_repl_step(Program& prog, const std::string& input) {
    prog.repl_step(input);
}

EMSCRIPTEN_BINDINGS(fire_script) {
    class_<Program>("FireScript")
        .constructor<>()
        .function("set_term_output_cb", &prog_set_term_output_cb)
        .function("set_quit_cb", &prog_set_quit_cb)
        .function("repl_init", &Program::repl_init)
        .function("repl_prompt", &Program::repl_prompt)
        .function("repl_step", &prog_repl_step)
        ;
}

#else  // !__EMSCRIPTEN__

int main(int argc, char* argv[])
{
    Program prog;
    prog.process_args(argv);
    prog.repl_init();
    prog.repl_loop();
    return 0;
}

#endif  // __EMSCRIPTEN__
