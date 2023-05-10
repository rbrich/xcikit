// type_index.h created on 2023-05-06 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_SCRIPT_TYPE_INDEX_H
#define XCI_SCRIPT_TYPE_INDEX_H

#include <xci/script/TypeInfo.h>

namespace xci::script {


/// Add the type and its underlying type / elem type / subtypes to the module,
/// unless they are builtin types.
Index make_type_index(Module& mod, const TypeInfo& type_info);

/// Get TypeIndex for a given TypeInfo
/// \param mod          Search this module for non-builtin types
/// \param type_info    The TypeInfo to be resolved
Index get_type_index(const Module& mod, const TypeInfo& type_info);

/// Get TypeInfo for a given TypeIndex
/// \param mod          Search this module for non-builtin types
/// \param type_idx     The TypeIndex to be resolved
const TypeInfo& get_type_info(const Module& mod, int32_t type_idx);


}  // namespace xci::script

#endif  // include guard
