// ModuleManager.h created on 2022-01-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_MODULE_MANAGER_H
#define XCI_SCRIPT_MODULE_MANAGER_H

#include "SymbolTable.h"  // Index
#include <xci/core/Vfs.h>
#include <string_view>
#include <map>

namespace xci::script {

class Interpreter;
class Module;

using ModulePtr = std::shared_ptr<Module>;


class ModuleManager {
public:
    ModuleManager(const core::Vfs& vfs, Interpreter& interpreter);

    ModulePtr import_module(const std::string& name);
    ModulePtr import_module(std::string_view name)
        { return import_module(std::string(name)); }
    ModulePtr import_module(const char* name)
        { return import_module(std::string(name)); }

    /// Create a new module, or replace existing one
    /// (The module name is always unique in the manager.)
    Index replace_module(const std::string& name);
    Index replace_module(const std::string& name, ModulePtr mod);

    /// Same as replace_module, but return the module instead of index
    ModulePtr make_module(const std::string& name) { return get_module(replace_module(name)); }

    ModulePtr get_module(Index idx) { return m_modules[idx]; }
    const Module& get_module(Index idx) const { return *m_modules[idx]; }
    const Module& get_module(const std::string& name) const { return *m_modules[m_module_names.at(name)]; }
    Index get_module_index(const Module& mod) const;
    size_t num_modules() const { return m_modules.size(); }

    Module& builtin_module() { return *m_modules.front(); }
    const Module& builtin_module() const { return const_cast<ModuleManager*>(this)->builtin_module(); }

    // drop all modules except builtin and std
    void clear(bool keep_std = true);

private:
    const core::Vfs& m_vfs;
    Interpreter& m_interpreter;
    std::vector<ModulePtr> m_modules;
    std::map<std::string, Index> m_module_names;  // map name to index
};


} // namespace xci::script

#endif // include guard
