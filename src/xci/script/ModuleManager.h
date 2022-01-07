// ModuleManager.h created on 2022-01-03 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2022 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_MODULE_MANAGER_H
#define XCI_SCRIPT_MODULE_MANAGER_H

#include <xci/core/Vfs.h>
#include <string_view>
#include <map>

namespace xci::script {

class Interpreter;
class Module;


class ModuleManager {
public:
    ModuleManager(const core::Vfs& vfs, Interpreter& interpreter);

    std::shared_ptr<Module> import_module(const std::string& name);
    std::shared_ptr<Module> import_module(std::string_view name)
        { return import_module(std::string(name)); }
    std::shared_ptr<Module> import_module(const char* name)
        { return import_module(std::string(name)); }

    bool add_module(const std::string& name, std::shared_ptr<Module> module);

    Module& builtin_module() { return *m_modules["builtin"]; }
    const Module& builtin_module() const { return const_cast<ModuleManager*>(this)->builtin_module(); }

private:
    const core::Vfs& m_vfs;
    Interpreter& m_interpreter;
    std::map<std::string, std::shared_ptr<Module>> m_modules;
};


} // namespace xci::script

#endif // include guard
