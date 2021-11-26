// FontLibrary.cpp created on 2018-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#include "FontLibrary.h"
#include "FontFace.h"
#include <xci/core/log.h>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace xci::text {

using namespace xci::core;


FontError::FontError(int error_code, const char* detail) :
    Error(fmt::format("FT_Error: {} detail: {}", error_code, detail)),
    m_error_code(error_code) {}


FontLibrary::FontLibrary()
{
    auto err = FT_Init_FreeType(&m_ft_library);
    if (err) {
        throw FontError(err, "FT_Init_FreeType");
    }
}


FontLibrary::~FontLibrary()
{
    auto err = FT_Done_FreeType(m_ft_library);
    if (err) {
        log::error("FT_Done_FreeType: {}", err);
        return;
    }
}


FontLibraryPtr FontLibrary::create_instance()
{
    return std::make_shared<FontLibrary>();
}


std::shared_ptr<FontLibrary> FontLibrary::default_instance()
{
    thread_local static auto instance = create_instance();
    return instance;
}


FontFacePtr FontLibrary::create_font_face()
{
    return std::make_unique<FontFace>(shared_from_this());
}


} // namespace xci::text
