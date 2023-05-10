// type_index.h created on 2023-05-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TYPE_INDEX_H
#define XCI_SCRIPT_TYPE_INDEX_H

#include <xci/script/TypeInfo.h>

namespace xci::script {

class ModuleManager;


/// Add the type and its underlying type / elem type / subtypes to the module,
/// unless they are builtin types.
Index make_type_index(Module& mod, const TypeInfo& type_info);

/// Get TypeIndex for a given TypeInfo
/// \param mm           The global ModuleManager with all modules imported in the program
/// \param type_info    The TypeInfo to be resolved
Index get_type_index(const ModuleManager& mm, const TypeInfo& type_info);

/// Get TypeInfo for a given TypeIndex
/// \param mm           The global ModuleManager with all modules imported in the program
/// \param type_idx     The TypeIndex to be resolved
/// \returns found TypeInfo or ti_unknown() if not found
const TypeInfo& get_type_info(const ModuleManager& mm, int32_t type_idx);

/// Get TypeInfo for a given TypeIndex
/// Same as `get_type_info` but crashes if not found.
/// For use in Machine where the index must always be valid.
const TypeInfo& get_type_info_unchecked(const ModuleManager& mm, int32_t type_idx);


}  // namespace xci::script

#endif  // include guard
