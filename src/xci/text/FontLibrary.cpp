// FontLibrary.cpp created on 2018-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FontLibrary.h"
#include "freetype/FtFontLibrary.h"
#include "freetype/FtFontFace.h"

#include <fmt/core.h>

namespace xci::text {


FontError::FontError(int error_code, const char* detail) :
    Error(fmt::format("FT_Error: {} detail: {}", error_code, detail)),
    m_error_code(error_code) {}


FontLibraryPtr FontLibrary::create_instance()
{
    return std::make_shared<FtFontLibrary>();
}


std::shared_ptr<FontLibrary> FontLibrary::default_instance()
{
    thread_local static auto instance = create_instance();
    return instance;
}


FontFacePtr FontLibrary::create_font_face()
{
    return std::make_unique<FtFontFace>(shared_from_this());
}


} // namespace xci::text
