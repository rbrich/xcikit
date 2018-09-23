// FontLibrary.cpp created on 2018-03-01, part of XCI toolkit
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

#include "FontLibrary.h"
#include "freetype/FtFontLibrary.h"
#include "freetype/FtFontFace.h"
#include <xci/core/format.h>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace xci {
namespace text {


FontError::FontError(int error_code, const char* detail) :
    Error(core::format("FT_Error: {} detail: {}", error_code, detail)),
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


}} // namespace xci::text
