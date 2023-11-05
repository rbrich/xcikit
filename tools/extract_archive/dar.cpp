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
#include <map>
#include <vector>

using namespace xci::core;
using namespace xci::core::argparser;
namespace fs = std::filesystem;


static void extract_entry(const std::string& name, const VfsFile& file, fs::path output_path)
{
    const auto entry_path = output_path / name;
    TermCtl& term = TermCtl::stdout_instance();
    term.print("Extracting file\t{fg:yellow}{}{t:normal} to {}\n", name, entry_path);
    auto content = file.content();
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


static bool is_wad_map_entry(const std::string& name)
{
    // ExMy or MAPxx
    return (name[0] == 'E' && std::isdigit((int)name[1]) &&
            name[2] == 'M' && std::isdigit((int)name[3]) && name[4] == 0) ||
           (std::memcmp(name.c_str(), "MAP", 3) == 0 &&
            std::isdigit((int)name[3]) && std::isdigit((int)name[4]) && name[5] == 0);
}


static bool is_wad_map_subentry(const std::string& name)
{
    return name == "THINGS" || name == "LINEDEFS" || name == "SIDEDEFS" ||
           name == "VERTEXES" || name == "SEGS" || name == "SSECTORS" ||
           name == "NODES" || name == "SECTORS" || name == "REJECT" ||
           name == "BLOCKMAP" || name == "BEHAVIOR";
}


/// Special handling of WADs ordered and non-uniquely named lumps.
/// Map the original lump names to virtual paths:
/// * "<lump name>" for normal entries
/// * "_1/<lump name>" for repeated lump names (_1 is the second occurrence, increments for each repetition)
/// * "_MAP01/<lump name>" for map lumps
/// The filename (without subdir) always matches the original lump name.
void extract_wad(VfsDirectory& vfs_dir, fs::path output_path)
{
    if (fs::exists(output_path / ".wad")) {
        log::warning("Not overwriting existing .wad at {}", output_path);
        return;
    }

    // Generating .wad while extracting, write as last file
    fs::create_directories(output_path);
    std::ofstream dot_wad(output_path / ".wad",
                          std::ios::out | std::ios::binary | std::ios::trunc);

    // .wad first line: IWAD or PWAD
    dot_wad << vfs_dir.type() << '\n';
    if (!dot_wad) {
        log::warning("Error writing {}", output_path / ".wad");
        return;
    }

    // .wad each entry: <lump name>\t<path>
    std::string subdir;  // virtual path for next entry
    std::map<std::string, int> repetition;  // counter for multiple lumps with same name
    for (const auto& entry : vfs_dir) {
        const auto entry_name = entry.name();
        auto entry_subdir = subdir;

        // reset subdir if this entry is not part of a map
        if (!is_wad_map_subentry(entry_name)) {
            subdir.clear();
        }
        // set subdir of this entry
        entry_subdir = subdir;
        // set subdir for following entries, if this entry is map starter
        if (is_wad_map_entry(entry_name)) {
            subdir = entry_name;
        }
        // add subdir for repeated names (_1, _2...)
        if (entry_subdir.empty()) {
            int& rep = repetition[entry_name];
            if (rep != 0)
                entry_subdir = std::to_string(rep);
            ++rep;
        }

        std::string virtual_path;
        if (!entry_subdir.empty()) {
            entry_subdir = '_' + entry_subdir;
            virtual_path = entry_subdir + '/';
        }
        virtual_path += entry.name();
        dot_wad << entry_name << '\t' << virtual_path << '\n';
        extract_entry(entry.name(), entry.file(), output_path / entry_subdir);
    }

    dot_wad.flush();
    dot_wad.close();
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
            for (const auto& entry : *vfs.mounts().back().vfs_dir) {
                term.print("{fg:yellow}{}{t:normal}\n", entry.name());
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
            VfsDirectory& vfs_dir = *vfs.mounts().back().vfs_dir;
            if (vfs_dir.type().substr(1) == "WAD") {
                extract_wad(vfs_dir, output_path);
            } else {
                for (const auto& entry : vfs_dir)
                    extract_entry(entry.name(), entry.file(), output_path);
            }
        } else {
            for (const char* name : entries)
                extract_entry(name, vfs.read_file(name), output_path);
        }
    }

    return 0;
}
