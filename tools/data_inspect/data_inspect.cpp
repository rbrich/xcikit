// data_inspect.cpp created on 2020-08-15 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020–2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

/// Data Inspector (dati) command line tool
/// Parses binary data files and shows their content in generic fashion,
/// with numeric tags and non-blob types converted to human-readable presentation.

#include <xci/core/ArgParser.h>
#include <xci/data/BinaryBase.h>
#include <xci/data/BinaryReader.h>
#include <xci/data/Schema.h>
#include <xci/core/TermCtl.h>
#include <xci/core/string.h>
#include <xci/core/bit.h>

#include <fstream>
#include <vector>
#include <optional>

using namespace xci::core;
using namespace xci::core::argparser;
using namespace xci::data;


static const char* type_to_cstr(uint8_t type)
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
    const auto expected_size = BinaryBase::size_by_type(type);
    if (expected_size != size_t(-1) && size != expected_size) {
        term.print("{fg:red}bad size {}{t:normal}", size);
        return;
    }

    switch (type) {
        case BinaryBase::Null:      term.print("{fg:yellow}null{t:normal}"); return;
        case BinaryBase::BoolFalse: term.print("{fg:yellow}false{t:normal}"); return;
        case BinaryBase::BoolTrue:  term.print("{fg:yellow}true{t:normal}"); return;
        case BinaryBase::Byte:      term.print("{fg:magenta}{}{t:normal}", int(*data)); return;
        case BinaryBase::UInt32:    term.print("{fg:magenta}{}{t:normal}", bit_copy<uint32_t>(data)); return;
        case BinaryBase::UInt64:    term.print("{fg:magenta}{}{t:normal}", bit_copy<uint64_t>(data)); return;
        case BinaryBase::Int32:     term.print("{fg:magenta}{}{t:normal}", bit_copy<int32_t>(data)); return;
        case BinaryBase::Int64:     term.print("{fg:magenta}{}{t:normal}", bit_copy<int64_t>(data)); return;
        case BinaryBase::Float32:   term.print("{fg:magenta}{}{t:normal}", bit_copy<float>(data)); return;
        case BinaryBase::Float64:   term.print("{fg:magenta}{}{t:normal}", bit_copy<double>(data)); return;
        case BinaryBase::VarInt:    term.print("{fg:yellow}varint{t:normal}"); return;
        case BinaryBase::Array:     term.print("{fg:yellow}array{t:normal}"); return;
        case BinaryBase::String:
            term.print("{fg:green}\"{}\"{t:normal}",
                    escape(std::string_view((const char*) data, size)));
            return;
        case BinaryBase::Binary:    term.print("{fg:yellow}(size {}){t:normal}", size); return;
        case BinaryBase::Master:
            term.print("{fg:yellow}(size {}){t:normal} {t:bold}{{{t:normal}", size); return;
        case BinaryBase::Control:   term.print("{fg:yellow}control{t:normal}"); return;
    }
    term.print("{fg:red}unknown{t:normal}");
}


static std::optional<int> int_value(uint8_t type, const std::byte* data, size_t size)
{
    switch (type) {
        case BinaryBase::Byte:      return int(*data);
        case BinaryBase::UInt32:    return (int) bit_copy<uint32_t>(data);
        case BinaryBase::UInt64:    return (int) bit_copy<uint64_t>(data);
        case BinaryBase::Int32:     return (int) bit_copy<int32_t>(data);
        case BinaryBase::Int64:     return (int) bit_copy<int64_t>(data);
        default:                    return {};
    }
}


int main(int argc, const char* argv[])
{
    std::string schema_file;
    std::vector<const char*> files;

    TermCtl& term = TermCtl::stdout_instance();

    ArgParser {
            Option("-h, --help", "Show help", show_help),
            Option("-s, --schema SCHEMA", "Schema file, used to describe the fields (names, types)", schema_file),
            Option("-- FILE ...", "Files to parse", files),
    } (argv);

    if (files.empty()) {
        term.print("{t:bold}{fg:yellow}No input files.{t:normal}\n");
    }

    Schema schema;
    if (!schema_file.empty()) {
        std::ifstream f(schema_file, std::ios::binary);
        try {
            BinaryReader reader(f);
            reader(schema);
            reader.finish_and_check();
        } catch (const ArchiveError& e) {
            term.print("{t:bold}{fg:red}Error reading schema: {}{t:normal}\n", e.what());
        }
    } else if (files.size() == 1 && std::string(files.back()).ends_with(".schema")) {
        // get Schema of .schema file
        schema("schema", schema);
    }

    for (const auto& filename : files) {
        term.print("{fg:yellow}{t:bold}{}{t:normal}\n", filename);
        std::ifstream f(filename, std::ios::binary);
        try {
            BinaryReader reader(f);

            // This is implicit, any other MAGIC/VERSION would throw
            term.print("CBDF (Chunked Binary Data Format), version 1\n");
            term.print("Flags: ({:x}) LittleEndian{}\n", int(reader.flags()),
                    reader.has_crc() ? ", ChecksumCrc32" : "");
            term.print("Size: {}\n", reader.root_group_size());

            bool eof = false;
            std::vector<const Schema::Struct*> struct_stack {&schema.struct_main()};
            std::map<std::string, int> last_int_values;  // for variant index
            while (!eof) {
                auto it = reader.generic_next();
                const Schema::Member* schema_member = struct_stack.back() ?
                        struct_stack.back()->member_by_key(it.key) : nullptr;
                const auto indent = (struct_stack.size() - 1) * 4;

                using What = BinaryReader::GenericNext::What;
                switch (it.what) {
                    case What::EnterGroup:
                    case What::DataItem:
                    case What::MetadataItem:
                        {
                            if (schema_member) {
                                if (schema_member->type.starts_with("variant ")) {
                                    // name of variant field refers to variant ID field in one of these ways:
                                    // * the name of variant ID field is same as the variant field itself
                                    // * the name of variant ID field is referred in brackets: `name[id_name]`
                                    std::string index_name;
                                    auto beg = schema_member->name.find('[');
                                    if (beg != std::string::npos) {
                                        ++beg;
                                        auto end = schema_member->name.find(']', beg);
                                        index_name = schema_member->name.substr(beg, end - beg);
                                    } else {
                                        index_name = schema_member->name;
                                    }
                                    // retrieve value of the variant ID
                                    auto index_value = last_int_values.find(index_name);
                                    if (index_value != last_int_values.end()) {
                                        auto* variant_schema = schema.struct_by_name(schema_member->type);
                                        if (variant_schema != nullptr)
                                            schema_member = variant_schema->member_by_key(uint8_t(index_value->second));
                                    }
                                }
                                if (it.what == What::DataItem) {
                                    auto opt_int = int_value(it.type, it.data.get(), it.size);
                                    if (opt_int)
                                        last_int_values[schema_member->name] = *opt_int;
                                }
                                term.print("{}{t:bold}{fg:cyan}{} ({}: {}){t:normal}: {} = ",
                                           std::string(indent, ' '),
                                           int(it.key), schema_member->name, schema_member->type,
                                           type_to_cstr(it.type));
                            } else {
                                term.print("{}{t:bold}{fg:cyan}{}{t:normal}: {} = ",
                                           std::string(indent, ' '),
                                           int(it.key), type_to_cstr(it.type));
                            }
                        }
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
                        if (it.what == What::EnterGroup) {
                            struct_stack.push_back(schema_member ?
                                schema.struct_by_name(schema_member->type) : nullptr);
                            last_int_values.clear();
                        }
                        break;
                    case What::LeaveGroup:
                        struct_stack.pop_back();
                        last_int_values.clear();
                        term.print("{t:bold}{}}}{t:normal}\n", std::string(indent - 4, ' '));
                        break;
                    case What::EnterMetadata:
                        term.print("{t:bold}{}Metadata:{t:normal}\n", std::string(indent, ' '));
                        break;
                    case What::LeaveMetadata:
                        term.print("{t:bold}{}Data:{t:normal}\n", std::string(indent, ' '));
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
