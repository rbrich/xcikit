// Module.h created on 2019-06-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_MODULE_H
#define XCI_SCRIPT_MODULE_H

#include "Value.h"
#include "SymbolTable.h"
#include "Class.h"
#include "Function.h"
#include <string>
#include <cstdint>

namespace xci::script {


/// Module is the translation unit - it contains functions and constants

class Module {
public:
    explicit Module(std::string name) : m_symtab(move(name)) { m_symtab.set_module(this); }
    Module() : Module("<module>") {}
    ~Module();

    const std::string& name() const { return m_symtab.name(); }

    SymbolPointer add_native_function(std::string&& name,
            std::vector<TypeInfo>&& params, TypeInfo&& retval,
            NativeDelegate native);

    template<class F>
    SymbolPointer add_native_function(std::string&& name, F&& fun) {
        auto w = native::AutoWrap{core::ToFunctionPtr(std::forward<F>(fun))};
        return add_native_function(std::move(name),
                w.param_types(), w.return_type(), w.native_wrapper());
    }

    template<class F>
    SymbolPointer add_native_function(std::string&& name, F&& fun, void* arg0) {
        auto w = native::AutoWrap(core::ToFunctionPtr(std::forward<F>(fun)), arg0);
        return add_native_function(std::move(name),
                w.param_types(), w.return_type(), w.native_wrapper());
    }

    // Imported modules
    // - lookup is reversed, first module is checked last
    // - index 0 should be builtin
    // - index 1 should be std
    // - imported modules are added in import order
    void add_imported_module(Module& module) { m_modules.push_back(&module); }
    Module& get_imported_module(size_t idx) const { return *m_modules[idx]; }
    Index get_imported_module_index(Module* module) const;
    size_t num_imported_modules() const { return m_modules.size(); }

    // Functions
    Index add_function(std::unique_ptr<Function>&& fn);
    Function& get_function(size_t idx) const { return *m_functions[idx]; }
    size_t num_functions() const { return m_functions.size(); }

    // Static values
    Index add_value(TypedValue&& value);
    const TypedValue& get_value(Index idx) const { return m_values[idx]; }
    size_t num_values() const { return m_values.size(); }

    // Type information
    Index add_type(TypeInfo type_info);
    const TypeInfo& get_type(Index idx) const { return m_types[idx]; }
    size_t num_types() const { return m_types.size(); }

    // Type classes
    Index add_class(std::unique_ptr<Class>&& cls);
    Class& get_class(size_t idx) const { return *m_classes[idx]; }
    size_t num_classes() const { return m_classes.size(); }

    // Instances
    Index add_instance(std::unique_ptr<Instance>&& inst);
    Instance& get_instance(size_t idx) const { return *m_instances[idx]; }
    size_t num_instances() const { return m_instances.size(); }

    // Top-level symbol table
    SymbolTable& symtab() { return m_symtab; }
    const SymbolTable& symtab() const { return m_symtab; }

    bool operator==(const Module& rhs) const;

private:
    std::vector<Module*> m_modules;
    std::vector<std::unique_ptr<Function>> m_functions;
    std::vector<std::unique_ptr<Class>> m_classes;
    std::vector<std::unique_ptr<Instance>> m_instances;
    std::vector<TypeInfo> m_types;
    TypedValues m_values;
    SymbolTable m_symtab;
};


} // namespace xci::script

#endif // include guard
