// Config.h created on 2023-11-08 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_CONFIG_H
#define XCI_CONFIG_H

#include <variant>
#include <filesystem>

namespace xci::config {

namespace fs = std::filesystem;


struct ConfigItem;

/// Retained config data
/// Config can be created and edited in-memory, then serialized to file.
/// Parsing facility is provided by `ConfigParser`, the supported syntax is documented there.
class Config {
public:
    bool parse_file(const fs::path& path);
    bool parse_string(const std::string& str);

    ConfigItem& operator[](const std::string& name) { return set(name); }
    const ConfigItem& operator[](const std::string& name) const { return get(name); }

    /// Add item to the back. Does not check uniquiness.
    ConfigItem& add(const std::string& name);

    /// Find item, or add new null item if not found.
    ConfigItem& set(const std::string& name);

    /// Get existing item. UB if doesn't exist.
    const ConfigItem& get(const std::string& name) const { return *get_next(name); }

    using const_iterator = std::vector<ConfigItem>::const_iterator;
    using iterator = std::vector<ConfigItem>::iterator;

    const_iterator get_next(const std::string& name, const_iterator prev = const_iterator{}) const;
    iterator get_next(const std::string& name, iterator prev = iterator{});

    const_iterator begin() const noexcept { return m_items.begin(); }
    const_iterator end() const noexcept { return m_items.end(); }
    iterator begin() noexcept { return m_items.begin(); }
    iterator end() noexcept { return m_items.end(); }

    const ConfigItem& front() const noexcept;
    const ConfigItem& back() const noexcept;
    ConfigItem& front() noexcept;
    ConfigItem& back() noexcept;

    size_t size() const;
    bool is_empty() const noexcept { return m_items.empty(); }

private:
    std::vector<ConfigItem> m_items;
};


using ConfigValue = std::variant<std::monostate, bool, int64_t, double, std::string, Config>;

struct ConfigItem {
    explicit ConfigItem(std::string name) : m_name(std::move(name)) {}

    const std::string& name() const { return m_name; }
    void set_name(std::string name) { m_name = std::move(name); }

    template <typename T>
    void operator= (T value) {
        m_value = std::forward<T>(value);
    }

    ConfigItem& operator[](const std::string& name) { return is_group()? as_group()[name] : m_value.emplace<Config>()[name]; }
    const ConfigItem& operator[](const std::string& name) const { return as_group()[name]; }


    // Strict comparison, without conversion. If the type doesn't match, returns false.
    bool operator== (bool value) const noexcept { return is_bool() && as_bool() == value; }
    bool operator== (int value) const noexcept { return is_int() && as_int() == value; }
    bool operator== (int64_t value) const noexcept { return is_int() && as_int() == value; }
    bool operator== (double value) const noexcept { return is_float() && as_float() == value; }
    bool operator== (const char* value) const noexcept { return is_string() && as_string() == value; }
    bool operator== (std::string_view value) const noexcept { return is_string() && as_string() == value; }

    // -------------------------------------------------------------------------
    // Check type of value

    /// The item is unset or discarded. Won't be serialized.
    bool is_null() const { return std::holds_alternative<std::monostate>(m_value); }

    bool is_bool() const { return std::holds_alternative<bool>(m_value); }
    bool is_int() const { return std::holds_alternative<int64_t>(m_value); }
    bool is_float() const { return std::holds_alternative<double>(m_value); }
    bool is_string() const { return std::holds_alternative<std::string>(m_value); }
    bool is_group() const { return std::holds_alternative<Config>(m_value); }

    // -------------------------------------------------------------------------
    // Access value - the actual type must match!

    bool as_bool() { return std::get<bool>(m_value); }
    int64_t as_int() { return std::get<int64_t>(m_value); }
    double as_float() { return std::get<double>(m_value); }
    std::string& as_string() { return std::get<std::string>(m_value); }
    Config& as_group() { return std::get<Config>(m_value); }

    bool as_bool() const { return std::get<bool>(m_value); }
    int64_t as_int() const { return std::get<int64_t>(m_value); }
    double as_float() const { return std::get<double>(m_value); }
    const std::string& as_string() const { return std::get<std::string>(m_value); }
    const Config& as_group() const { return std::get<Config>(m_value); }

    // -------------------------------------------------------------------------
    // Convert value

    // For int and float, returns false if 0, otherwise true.
    // String is converted to true only if "true", otherwise false.
    bool to_bool() const { return std::get<bool>(m_value); }

    // Bool is converted to 0 / 1.
    // Float is truncated to int.
    // String is parsed to int if possible, otherwise 0 (atoi).
    int64_t to_int() const { return std::get<int64_t>(m_value); }

    // Bool is converted to 0.0 / 1.0.
    // String is parsed to float, 0 on failure.
    double to_float() const { return std::get<double>(m_value); }

    // Bool is converted to "false" / "true".
    // Int/float is converted to string.
    std::string to_string() const { return std::get<std::string>(m_value); }

private:
    std::string m_name;
    ConfigValue m_value;
};


}  // namespace xci::config

#endif  // XCI_CONFIG_H
