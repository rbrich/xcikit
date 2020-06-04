// Module.h created on 2019-06-12 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2019–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_MODULE_H
#define XCI_SCRIPT_MODULE_H

#include "Value.h"
#include "SymbolTable.h"
#include "Class.h"
#include "Function.h"
#include <xci/core/container/IndexedMap.h>
#include <string>
#include <map>
#include <cstdint>

namespace xci::script {

using xci::core::IndexedMap;


/// Module is the translation unit - it contains functions and constants

class Module {
public:
    using FunctionId = IndexedMap<Function>::WeakIndex;
    using ClassId = IndexedMap<Class>::WeakIndex;
    using InstanceId = IndexedMap<Instance>::WeakIndex;

    explicit Module(std::string name) : m_symtab(move(name)) { m_symtab.set_module(this); }
    Module() : Module("<module>") { m_symtab.set_module(this); }
    ~Module();
    Module(Module&&) = delete;
    Module& operator =(Module&&) = delete;

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
    FunctionId add_function(Function&& fn);
    const Function& get_function(Index id) const { return m_functions[id]; }
    Function& get_function(Index id) { return m_functions[id]; }
    size_t num_functions() const { return m_functions.size(); }

    // Static values
    Index add_value(TypedValue&& value);
    const TypedValue& get_value(Index idx) const { return m_values[idx]; }
    Index find_value(const TypedValue& value) const;
    size_t num_values() const { return m_values.size(); }

    // Type information
    Index add_type(TypeInfo type_info);
    const TypeInfo& get_type(Index idx) const { return m_types[idx]; }
    Index find_type(const TypeInfo& type_info) const;
    void set_type(Index idx, TypeInfo&& type_info) { m_types[idx] = move(type_info); }
    size_t num_types() const { return m_types.size(); }

    // Type classes
    ClassId add_class(Class&& cls);
    const Class& get_class(size_t idx) const { return m_classes[idx]; }
    Class& get_class(size_t idx) { return m_classes[idx]; }
    size_t num_classes() const { return m_classes.size(); }

    // Instances
    InstanceId add_instance(Instance&& inst);
    const Instance& get_instance(size_t idx) const { return m_instances[idx]; }
    Instance& get_instance(size_t idx) { return m_instances[idx]; }
    size_t num_instances() const { return m_instances.size(); }

    // Top-level symbol table
    SymbolTable& symtab() { return m_symtab; }
    const SymbolTable& symtab() const { return m_symtab; }

    // Specialized generic functions
    void add_spec_function(SymbolPointer gen_fn, Index spec_fn_idx);
    std::vector<Index> get_spec_functions(SymbolPointer gen_fn);

    // Serialization
    bool save(const std::string& filename);

    bool operator==(const Module& rhs) const;

private:
    std::vector<Module*> m_modules;
    IndexedMap<Function> m_functions;
    IndexedMap<Class> m_classes;
    IndexedMap<Instance> m_instances;
    std::vector<TypeInfo> m_types;
    TypedValues m_values;
    SymbolTable m_symtab;

    // Specialized generic functions
    // * SymbolPointer points to original generic function
    // * Index is function index in this module
    std::multimap<SymbolPointer, Index> m_spec_functions;
};


} // namespace xci::script

#endif // include guard
