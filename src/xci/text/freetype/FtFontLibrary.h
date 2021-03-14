// FtFontLibrary.h created on 2018-09-23 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_FREETYPE_FONTLIBRARY_H
#define XCI_TEXT_FREETYPE_FONTLIBRARY_H

#include <xci/text/FontLibrary.h>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace xci::text {


class FtFontLibrary: public FontLibrary {
public:
    FtFontLibrary();
    ~FtFontLibrary() override;

    FT_Library ft_library() const { return m_ft_library; }

private:
    FT_Library m_ft_library = nullptr;
};


} // xci::text

#endif // XCI_TEXT_FREETYPE_FONTLIBRARY_H
