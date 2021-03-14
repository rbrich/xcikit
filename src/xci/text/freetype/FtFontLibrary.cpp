// FtFontLibrary.cpp created on 2018-09-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FtFontLibrary.h"
#include <xci/core/log.h>
#include <xci/core/error.h>

namespace xci::text {

using namespace xci::core;


FtFontLibrary::FtFontLibrary()
{
    auto err = FT_Init_FreeType(&m_ft_library);
    if (err) {
        throw FontError(err, "FT_Init_FreeType");
    }
}


FtFontLibrary::~FtFontLibrary()
{
    auto err = FT_Done_FreeType(m_ft_library);
    if (err) {
        log::error("FT_Done_FreeType: {}", err);
        return;
    }
}


} // xci::text
