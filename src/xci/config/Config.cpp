// Config.cpp created on 2023-11-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Config.h"
#include "ConfigParser.h"
#include <xci/core/string.h>

#include <deque>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <cstdlib>  // atoi, strtod

namespace xci::config {

using xci::core::escape_utf8;


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


void Config::dump(std::ostream& os) const
{
    class DumpingVisitor {
    public:
        explicit DumpingVisitor(std::ostream& os) : os(os) {}

        void dump_items(const Config& v) {
            for (const ConfigItem& item : v) {
                if (item.is_null())
                    continue;
                os << indent() << item.name() << ' ';
                item.visit(*this);
            }
        }

        void operator()(std::monostate v) const {}
        void operator()(bool v) const { os << std::boolalpha << v << '\n'; }
        void operator()(int64_t v) const { os << v << '\n'; }
        void operator()(double v) const { os << v << '\n'; }
        void operator()(const std::string& v) const { os << '"' << escape_utf8(v) << '"' << '\n'; }
        void operator()(const Config& v) {
            ++ m_indent;
            os << '{' << '\n';
            dump_items(v);
            -- m_indent;
            os << indent() << '}' << '\n';
        }

    private:
        std::string indent() const { return std::string(m_indent * 2, ' '); }
        std::ostream& os;
        int m_indent = 0;
    };
    DumpingVisitor visitor(os);
    visitor.dump_items(*this);
}


std::string Config::dump() const
{
    std::stringstream os;
    dump(os);
    return os.str();
}


bool Config::dump_to_file(const fs::path& path) const
{
    std::ofstream f(path);
    if (!f)
        return false;
    dump(f);
    return bool(f);
}


bool ConfigItem::to_bool() const
{
    return std::visit([](auto&& v) -> bool {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate> || std::is_same_v<T, Config>)
            return false;
        else if constexpr (std::is_same_v<T, std::string>)
            return v == "true";
        else
            return bool(v);
    }, m_value);
}


int64_t ConfigItem::to_int() const
{
    return std::visit([](auto&& v) -> int64_t {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate> || std::is_same_v<T, Config>)
            return 0;
        else if constexpr (std::is_same_v<T, std::string>)
            return std::atoi(v.c_str());
        else
            return int64_t(v);
    }, m_value);
}


double ConfigItem::to_float() const
{
    return std::visit([](auto&& v) -> double {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate> || std::is_same_v<T, Config>)
            return 0.0;
        else if constexpr (std::is_same_v<T, std::string>)
            return std::strtod(v.c_str(), nullptr);
        else
            return double(v);
    }, m_value);
}


std::string ConfigItem::to_string() const
{
    return std::visit([](auto&& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate> || std::is_same_v<T, Config>) {
            return {};
        } else {
            std::stringstream os;
            os << std::boolalpha << v;
            return os.str();
        }
    }, m_value);
}


}  // namespace xci::config
