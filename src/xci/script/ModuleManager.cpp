// ModuleManager.cpp created on 2022-01-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "ModuleManager.h"
#include "Module.h"
#include "Interpreter.h"
#include "Builtin.h"
#include "Error.h"
#include <fmt/core.h>

namespace xci::script {


ModuleManager::ModuleManager(const core::Vfs& vfs, Interpreter& interpreter)
        : m_vfs(vfs), m_interpreter(interpreter)
{
    m_modules.emplace_back(std::make_shared<BuiltinModule>(*this));
    m_module_names.try_emplace("builtin", 0);
}


std::shared_ptr<Module> ModuleManager::import_module(const std::string& name)
{
    auto it = m_module_names.try_emplace(name, 0);
    if (it.second) {
        // import module
        const std::string path = fmt::format("script/{}.fire", name);
        auto f = m_vfs.read_file(path);
        if (!f)
            throw ImportError(name);
        auto content = f.content();
        auto& source_manager = m_interpreter.source_manager();
        auto file_id = source_manager.add_source(path, content->string());
        m_modules.emplace_back(m_interpreter.build_module(name, file_id));
        it.first->second = m_modules.size() - 1;
        return m_modules.back();
    }
    // already existed
    return m_modules[it.first->second];
}


Index ModuleManager::add_module(const std::string& name, std::shared_ptr<Module> mod)  // NOLINT(performance-unnecessary-value-param)
{
    auto it = m_module_names.try_emplace(name, 0);
    if (it.second) {
        // added
        m_modules.emplace_back(std::move(mod));
        it.first->second = m_modules.size() - 1;
        return it.first->second;
    }
    // already existed
    return it.first->second;
}


Index ModuleManager::get_module_index(const Module& mod) const
{
    auto it = find_if(m_modules.begin(), m_modules.end(),
                      [&mod](const std::shared_ptr<Module>& a){ return &mod == a.get(); });
    if (it == m_modules.end())
        return no_index;
    return it - m_modules.begin();
}


void ModuleManager::clear(bool keep_std)
{
    std::shared_ptr<Module> builtin = std::move(m_modules[0]);
    std::shared_ptr<Module> std;
    if (keep_std && m_module_names.contains("std"))
        std = std::move(m_modules[m_module_names["std"]]);
    m_modules.clear();
    m_module_names.clear();
    add_module("builtin", std::move(builtin));
    if (std)
        add_module("std", std::move(std));
}


} // namespace xci::script
