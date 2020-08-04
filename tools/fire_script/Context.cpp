// Context.cpp created on 2020-01-11 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2020 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "Context.h"

namespace xci::script::tool {


Context& context()
{
    static Context ctx;
    return ctx;
}


} // namespace xci::script::repl
