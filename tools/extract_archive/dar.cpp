// dar.cpp created on 2023-11-05 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// DAR archive extractor (dar) command line tool
/// Also extracts WAD and ZIP (all formats supported by Vfs).

#include <xci/core/Vfs.h>
#include <xci/core/ArgParser.h>
#include <xci/core/TermCtl.h>
#include <xci/core/string.h>
#include <xci/core/log.h>

#include <filesystem>
#include <fstream>
#include <vector>

using namespace xci::core;
using namespace xci::core::argparser;
namespace fs = std::filesystem;


void extract_entry(const Vfs& vfs, const std::string& entry, fs::path output_path)
{
    const auto entry_path = output_path / entry;
    TermCtl& term = TermCtl::stdout_instance();
    term.print("Extracting file\t{fg:yellow}{}{t:normal} to {}\n", entry, entry_path);
    auto f = vfs.read_file(entry);
    auto content = f.content();
    if (content) {
        fs::create_directories(entry_path.parent_path());
        if (fs::exists(entry_path)) {
            log::warning("File exists, skipping: {}", entry_path);
            return;
        }
        std::ofstream of(entry_path);
        if (of) {
            of.write((const char*) content->data(), content->size());
        } else {
            log::error("Cannot open target file {}", entry_path);
        }
    }
}


int main(int argc, const char* argv[])
{
    std::vector<const char*> files;
    std::vector<const char*> entries;
    std::string output_dir;
    bool list_entries = false;

    // silence logging
    Logger::init(Logger::Level::Warning);

    TermCtl& term = TermCtl::stdout_instance();

    ArgParser {
            Option("-h, --help", "Show help", show_help),
            Option("-l, --list", "List entries, do not extract", list_entries),
            Option("-e, --entry ENTRY ...", "Extract selected entries (file names in archive)", entries),
            Option("-o, --output DIR", "Output directory for extracted files "
                                       "(default: archive path without extension)", output_dir),
            Option("-- ARCHIVE ...", "Archives to extract", files),
    } (argv);

    if (files.empty()) {
        term.print("{t:bold}{fg:yellow}No input files.{t:normal}\n");
    }

    Vfs vfs;
    for (const auto filename : files) {
        term.print("{t:bold}Extracting archive\t{fg:yellow}{}{t:normal}\n", filename);
        if (!vfs.mount(filename)) {
            term.print("{t:bold}{fg:red}Could not mount {}{t:normal}\n", filename);
            continue;
        }

        if (list_entries) {
            for (const auto& name : *vfs.mounts().back().vfs_dir) {
                term.print("{fg:yellow}{}{t:normal}\n", name);
            }
            continue;
        }

        fs::path output_path = output_dir;
        if (output_dir.empty()) {
            output_path = filename;
            if (output_path.extension().empty())
                output_path.replace_extension(".extracted");
            else
                output_path.replace_extension();
        }

        if (entries.empty()) {
            for (const auto& name : *vfs.mounts().back().vfs_dir)
                extract_entry(vfs, name, output_path);
        } else {
            for (const auto entry : entries)
                extract_entry(vfs, entry, output_path);
        }
    }

    return 0;
}
