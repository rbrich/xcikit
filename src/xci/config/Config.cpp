// Config.cpp created on 2023-11-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Config.h"
#include "ConfigParser.h"
#include <deque>

namespace xci::config {


class RetainedConfigParser final : public ConfigParser {
public:
    explicit RetainedConfigParser(Config& config) : m_config(&config) {}

protected:
    void name(const std::string& name) override { m_config->add(name); }
    void bool_value(bool value) override { m_config->back() = value; }
    void int_value(int64_t value) override { m_config->back() = value; }
    void float_value(double value) override { m_config->back() = value; }
    void string_value(std::string value) override { m_config->back() = std::move(value); }

    void begin_group() override {
        m_config_stack.push_back(m_config);
        m_config->back() = Config();
        m_config = &m_config->back().as_group();
    }
    void end_group() override {
        m_config = m_config_stack.back();
        m_config_stack.pop_back();
    }

private:
    Config* m_config;
    std::deque<Config*> m_config_stack;
};


ConfigItem& Config::add(const std::string& name)
{
    m_items.push_back(ConfigItem(name));
    return m_items.back();
}


ConfigItem& Config::set(const std::string& name)
{
    auto it = get_next(name);
    if (it == end())
        return add(name);
    return *it;
}


template <class C, class I>
static I _get_next(C& c, const std::string& name, I prev)
{
    if (prev == I{} || prev == c.end())
        prev = c.begin();
    else
        ++prev;
    return std::find_if(prev, c.end(),
        [&name](const ConfigItem& v) { return v.name() == name; });
}


auto Config::get_next(const std::string& name, const_iterator prev) const -> const_iterator
{
    return _get_next(*this, name, prev);
}


auto Config::get_next(const std::string& name, iterator prev) -> iterator
{
    return _get_next(*this, name, prev);
}


ConfigItem& Config::back() noexcept { return m_items.back(); }
const ConfigItem& Config::back() const noexcept { return m_items.back(); }
ConfigItem& Config::front() noexcept { return m_items.front(); }
const ConfigItem& Config::front() const noexcept { return m_items.front(); }
size_t Config::size() const { return m_items.size(); }


bool Config::parse_file(const fs::path& path)
{
    RetainedConfigParser p(*this);
    return p.parse_file(path);
}


bool Config::parse_string(const std::string& str)
{
    RetainedConfigParser p(*this);
    return p.parse_string(str);
}


}  // namespace xci::config
