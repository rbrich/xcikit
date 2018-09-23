// FtFontLibrary.h created on 2018-09-23, part of XCI toolkit
// Copyright 2018 Radek Brich
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
