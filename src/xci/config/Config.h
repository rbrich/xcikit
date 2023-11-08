// Config.h created on 2023-11-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CONFIG_H
#define XCI_CONFIG_H

namespace xci::config {

struct ConfigItem;


/// Retained config data
/// Config can be created and edited in-memory, then serialized to file.
/// Parsing facility is provided by `ConfigParser`, the supported syntax is documented there.
class Config {
public:

private:
    std::vector<ConfigItem> m_items;
};


struct ConfigItem {
    std::string name;
    std::variant<bool, int64_t, double, std::string, Config> value;
};


}  // namespace xci::config

#endif  // XCI_CONFIG_H
