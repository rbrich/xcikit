// file.h created on 2018-03-29 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018, 2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CORE_FILE_H
#define XCI_CORE_FILE_H

#include "Buffer.h"
#include <string>
#include <string_view>
#include <optional>
#include <filesystem>

namespace xci::core {

namespace fs = std::filesystem;


// Try to read whole content of a file.
// Returns empty string in case of any error.
std::optional<std::string> read_text_file(const fs::path& pathname);
std::optional<std::string> read_text_file(std::istream& stream);
BufferPtr read_binary_file(const fs::path& pathname);
BufferPtr read_binary_file(std::istream& stream);

/// Write string to FD (in a loop, handling EINTR).
/// \returns false on error (check errno), true on success
bool write(int fd, std::string_view s);


}  // namespace xci::core

#endif //XCI_CORE_FILE_H
