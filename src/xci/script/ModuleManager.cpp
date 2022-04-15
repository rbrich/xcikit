// ModuleManager.cpp created on 2022-01-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
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
    m_modules.try_emplace("builtin", std::make_shared<BuiltinModule>(*this));
}


std::shared_ptr<Module> ModuleManager::import_module(const std::string& name)
{
    if (!m_modules.contains(name)) {
        std::string path = fmt::format("script/{}.fire", name);
        auto f = m_vfs.read_file(path);
        if (!f)
            throw ImportError(name);
        auto content = f.content();
        auto& source_manager = m_interpreter.source_manager();
        auto file_id = source_manager.add_source(path, content->string());
        m_modules.try_emplace(name, m_interpreter.build_module(name, file_id));
    }

    return m_modules[name];
}


bool ModuleManager::add_module(const std::string& name, const std::shared_ptr<Module>& module)
{
    auto it = m_modules.try_emplace(name, module);
    return it.second;
}


} // namespace xci::script
