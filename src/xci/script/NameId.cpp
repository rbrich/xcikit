// NameId.cpp created on 2023-09-22 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2023 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "NameId.h"

namespace xci::script {


static thread_local core::StringPool tl_string_pool;


core::StringPool& NameId::string_pool()
{
    return tl_string_pool;
}


} // namespace xci::script
