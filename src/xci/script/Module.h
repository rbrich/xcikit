// Module.h created on 2019-06-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_MODULE_H
#define XCI_SCRIPT_MODULE_H

#include "Value.h"
#include "SymbolTable.h"
#include "Class.h"
#include "Function.h"
#include "ModuleManager.h"
#include <xci/core/container/IndexedMap.h>
#include <string>
#include <map>
#include <cstdint>

namespace xci::script {

using xci::core::IndexedMap;


/// Module is the translation unit - it contains functions and constants

class Module {
public:
    using WeakFunctionId = IndexedMap<Function>::WeakIndex;
    using FunctionIdx = IndexedMap<Function>::Index;
    using ScopeIdx = IndexedMap<Scope>::Index;
    using WeakClassId = IndexedMap<Class>::WeakIndex;
    using ClassIdx = IndexedMap<Class>::Index;
    using WeakInstanceId = IndexedMap<Instance>::WeakIndex;
    using InstanceIdx = IndexedMap<Instance>::Index;

    explicit Module(ModuleManager& module_manager, NameId name = intern("<module>"))
        : m_module_manager(&module_manager), m_symtab(name)
        { init(); }
    Module() : m_symtab(intern("<module>")) { m_symtab.set_module(this); }  // only for serialization
    ~Module();
    Module(Module&&) = delete;
    Module& operator =(Module&&) = delete;

    NameId name() const { return m_symtab.name(); }
    const ModuleManager& module_manager() const { return *m_module_manager; }

    SymbolPointer add_native_function(NameId name,
            TypeInfo&& param, TypeInfo&& retval, NativeDelegate native);

    SymbolPointer add_native_function(std::string_view name,
            TypeInfo&& param, TypeInfo&& retval, NativeDelegate native)
    { return add_native_function(intern(name), std::move(param), std::move(retval), native); }

    template<class F>
    SymbolPointer add_native_function(std::string_view name, F&& fun) {
        auto w = native::AutoWrap{core::ToFunctionPtr(std::forward<F>(fun))};
        return add_native_function(intern(name),
                w.param_type(), w.return_type(), w.native_wrapper());
    }

    template<class F>
    SymbolPointer add_native_function(std::string_view name, F&& fun, void* arg0) {
        auto w = native::AutoWrap(core::ToFunctionPtr(std::forward<F>(fun)), arg0);
        return add_native_function(intern(name),
                w.param_type(), w.return_type(), w.native_wrapper());
    }

    // Imported modules
    // - lookup is reversed, first module is checked last
    // - index 0 should be builtin
    // - index 1 should be std
    // - imported modules are added in import order
    Index import_module(std::string_view name) { return import_module(intern(name)); }
    Index import_module(NameId name);
    Index add_imported_module(std::shared_ptr<Module> mod);
    Module& get_imported_module(Index idx) const { return *m_modules[idx]; }
    Index get_imported_module_index(Module* mod) const;
    Index get_imported_module_index(NameId name) const;
    Size num_imported_modules() const { return Size(m_modules.size()); }

    // Functions
    WeakFunctionId add_function(Function&& fn);
    const Function* get_function(WeakFunctionId id) const { return m_functions.get(id); }
    const Function& get_function(FunctionIdx id) const { return m_functions[id]; }
    Function& get_function(FunctionIdx id) { return m_functions[id]; }
    Function& get_main_function() { return m_functions[0]; }
    WeakFunctionId find_function(NameId name) const;
    Size num_functions() const { return Size(m_functions.size()); }

    // Scopes
    ScopeIdx add_scope(Scope&& scope);
    const Scope& get_scope(ScopeIdx id) const { return m_scopes[id]; }
    Scope& get_scope(ScopeIdx id) { return m_scopes[id]; }
    Scope& get_main_scope() { return m_scopes[0]; }
    Size num_scopes() const { return Size(m_scopes.size()); }

    // Static values
    Index add_value(TypedValue&& value);
    const TypedValue& get_value(Index idx) const { return m_values[idx]; }
    Index find_value(const TypedValue& value) const;
    Size num_values() const { return Size(m_values.size()); }

    // Type information
    Index add_type(TypeInfo type_info);
    void update_type(Index index, TypeInfo type_info);
    const TypeInfo& get_type(Index idx) const { return m_types[idx]; }
    Index find_type(const TypeInfo& type_info) const;
    Size num_types() const { return Size(m_types.size()); }

    // Type classes
    WeakClassId add_class(Class&& cls);
    const Class& get_class(ClassIdx idx) const { return m_classes[idx]; }
    Class& get_class(ClassIdx idx) { return m_classes[idx]; }
    Size num_classes() const { return Size(m_classes.size()); }

    // Instances
    WeakInstanceId add_instance(Instance&& inst);
    const Instance& get_instance(InstanceIdx idx) const { return m_instances[idx]; }
    Instance& get_instance(InstanceIdx idx) { return m_instances[idx]; }
    Size num_instances() const { return Size(m_instances.size()); }

    // Top-level symbol table
    SymbolTable& symtab() { return m_symtab; }
    const SymbolTable& symtab() const { return m_symtab; }

    // Find symbol table by qualified function name
    SymbolTable& symtab_by_qualified_name(std::string_view name);

    // Specialized generic functions
    void add_spec_function(SymbolPointer gen_fn, Index spec_scope_idx);
    std::vector<Index> get_spec_functions(SymbolPointer gen_fn);

    // Specialized generic instances
    void add_spec_instance(SymbolPointer gen_inst, Index spec_inst_idx);
    std::vector<Index> get_spec_instances(SymbolPointer gen_inst);

    // Serialization
    bool save_to_file(const std::string& filename);
    bool load_from_file(const std::string& filename);
    bool write_schema_to_file(const std::string& filename);

    template<class Archive>
    void save(Archive& ar) const {
        ar("name", name());
    }

    // load() is implemented inside load_from_file in a wrapper class

    bool operator==(const Module& rhs) const;

private:
    void init();

    ModuleManager* m_module_manager = nullptr;
    std::vector<std::shared_ptr<Module>> m_modules;
    IndexedMap<Function> m_functions;
    IndexedMap<Scope> m_scopes;
    IndexedMap<Class> m_classes;
    IndexedMap<Instance> m_instances;
    std::vector<TypeInfo> m_types;
    TypedValues m_values;
    SymbolTable m_symtab;

    // Specialized generic functions
    // * SymbolPointer points to original generic function
    // * Index is scope index in this module
    std::multimap<SymbolPointer, Index> m_spec_functions;

    // Specialized generic instances
    // * SymbolPointer points to original generic instance
    // * Index is instance index in this module
    std::multimap<SymbolPointer, Index> m_spec_instances;
};


} // namespace xci::script

#endif // include guard
