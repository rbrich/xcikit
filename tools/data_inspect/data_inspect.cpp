// data_inspect.cpp created on 2020-08-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// Data Inspector (dati) command line tool
/// Parses binary data files and shows their content in generic fashion,
/// with numeric tags and non-blob types converted to human-readable presentation.

#include <xci/core/ArgParser.h>
#include <xci/data/BinaryBase.h>
#include <xci/data/BinaryReader.h>
#include <xci/core/TermCtl.h>
#include <xci/core/string.h>

#include <fstream>

using namespace xci::core;
using namespace xci::core::argparser;
using namespace xci::data;


static std::string type_to_str(uint8_t type)
{
    switch (type) {
        case BinaryBase::Null:      return "Null";
        case BinaryBase::BoolFalse: return "Bool";
        case BinaryBase::BoolTrue:  return "Bool";
        case BinaryBase::Byte:      return "Byte";
        case BinaryBase::UInt32:    return "UInt32";
        case BinaryBase::UInt64:    return "UInt64";
        case BinaryBase::Int32:     return "Int32";
        case BinaryBase::Int64:     return "Int64";
        case BinaryBase::Float32:   return "Float32";
        case BinaryBase::Float64:   return "Float64";
        case BinaryBase::VarInt:    return "Varint";
        case BinaryBase::Array:     return "Array";
        case BinaryBase::String:    return "String";
        case BinaryBase::Binary:    return "Binary";
        case BinaryBase::Master:    return "Master";
        case BinaryBase::Control:   return "Control";
    }
    return "Unknown";
}


static void print_data(TermCtl& term, uint8_t type, const std::byte* data, size_t size)
{
    auto expected_size = BinaryBase::size_by_type(type);
    if (expected_size != size_t(-1) && size != expected_size) {
        term.print("{fg:red}bad size {}{t:normal}", size);
        return;
    }

    switch (type) {
        case BinaryBase::Null:      term.print("{fg:yellow}null{t:normal}"); return;
        case BinaryBase::BoolFalse: term.print("{fg:yellow}false{t:normal}"); return;
        case BinaryBase::BoolTrue:  term.print("{fg:yellow}true{t:normal}"); return;
        case BinaryBase::Byte:      term.print("{fg:magenta}{}{t:normal}", int(*data)); return;
        case BinaryBase::UInt32:    term.print("{fg:magenta}{}{t:normal}", uint32_t(*data)); return;
        case BinaryBase::UInt64:    term.print("{fg:magenta}{}{t:normal}", uint64_t(*data)); return;
        case BinaryBase::Int32:     term.print("{fg:magenta}{}{t:normal}", int32_t(*data)); return;
        case BinaryBase::Int64:     term.print("{fg:magenta}{}{t:normal}", uint32_t(*data)); return;
        case BinaryBase::Float32:   term.print("{fg:magenta}{}{t:normal}", float(*data)); return;
        case BinaryBase::Float64:   term.print("{fg:magenta}{}{t:normal}", double(*data)); return;
        case BinaryBase::VarInt:    term.print("{fg:yellow}varint{t:normal}"); return;
        case BinaryBase::Array:     term.print("{fg:yellow}array{t:normal}"); return;
        case BinaryBase::String:
            term.print("{fg:green}\"{}\"{t:normal}",
                    escape(std::string((const char*) data, size)));
            return;
        case BinaryBase::Binary:    term.print("{fg:yellow}(size {}){t:normal}", size); return;
        case BinaryBase::Master:
            term.print("{fg:yellow}(size {}){t:normal} {t:bold}{{{t:normal}", size); return;
        case BinaryBase::Control:   term.print("{fg:yellow}control{t:normal}"); return;
    }
    term.print("{fg:red}unknown{t:normal}");
}


int main(int argc, const char* argv[])
{
    std::vector<const char*> files;

    TermCtl& term = TermCtl::stdout_instance();

    ArgParser {
            Option("-h, --help", "Show help", show_help),
            Option("-- FILE ...", "Files to parse", files),
    } (argv);

    if (files.empty()) {
        term.print("{t:bold}{fg:yellow}No input files.{t:normal}\n");
    }

    for (const auto& filename : files) {
        term.print("{fg:yellow}{t:bold}{}{t:normal}\n", filename);
        std::ifstream f(filename, std::ios::binary);
        try {
            xci::data::BinaryReader reader(f);

            // This is implicit, any other MAGIC/VERSION would throw
            term.print("CBDF (Chunked Binary Data Format), version 1\n");
            term.print("Flags: ({:x}) LittleEndian{}\n", int(reader.flags()),
                    reader.has_crc() ? ", ChecksumCrc32" : "");
            term.print("Size: {}\n", reader.root_group_size());

            int indent = 0;
            bool eof = false;
            while (!eof) {
                auto it = reader.generic_next();
                using What = BinaryReader::GenericNext::What;
                switch (it.what) {
                    case What::EnterGroup:
                    case What::DataItem:
                    case What::MetadataItem:
                        term.print("{}{t:bold}{fg:cyan}{}{t:normal}: {} = ",
                                std::string(indent * 4, ' '),
                                int(it.key), type_to_str(it.type));
                        print_data(term, it.type, it.data.get(), it.size);
                        if (it.what == What::MetadataItem) {
                            if (it.key == 1 && it.type == BinaryBase::UInt32) {
                                uint32_t stored_crc = 0;
                                std::memcpy(&stored_crc, it.data.get(), it.size);
                                if (reader.crc() == stored_crc)
                                    term.print(" {t:bold}{fg:green}(CRC32: OK){t:normal}");
                                else
                                    term.print(" {t:bold}{fg:red}(CRC32: expected {}){t:normal}",
                                            reader.crc());
                            }
                        }
                        term.print("\n");
                        if (it.what == What::EnterGroup)
                            ++ indent;
                        break;
                    case What::LeaveGroup:
                        -- indent;
                        term.print("{t:bold}{}}}{t:normal}\n", std::string(indent * 4, ' '));
                        break;
                    case What::EnterMetadata:
                        term.print("{t:bold}{}Metadata:{t:normal}\n", std::string(indent * 4, ' '));
                        break;
                    case What::LeaveMetadata:
                        term.print("{t:bold}{}Data:{t:normal}\n", std::string(indent * 4, ' '));
                        break;
                    case What::EndOfFile:
                        eof = true;
                        break;
                }
            }
        } catch (const ArchiveError& e) {
            term.print("{t:bold}{fg:red}{}{t:normal}\n", e.what());
        }
    }

    return 0;
}
