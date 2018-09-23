// FontLibrary.h created on 2018-03-01, part of XCI toolkit
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

#ifndef XCI_TEXT_FONTLIBRARY_H
#define XCI_TEXT_FONTLIBRARY_H

#include <memory>
#include <vector>
#include <string>
#include <xci/core/error.h>

namespace xci::text {


/// Fatal errors in font library (FreeType)
class FontError: public core::Error {
public:
    FontError(int error_code, const char* detail);

    // FreeType error code (FT_Error)
    int error_code() const { return m_error_code; }

private:
    int m_error_code;
};


class FontLibrary;
using FontLibraryPtr = std::shared_ptr<FontLibrary>;
class FontFace;
using FontFacePtr = std::unique_ptr<FontFace>;


/// FontLibrary is a factory for FontFace objects.
///
/// By default, it has one instance per thread, unless you construct
/// new instances explicitly. In that case, it's up to you how you manage
/// their lifetime. But keep in mind that the FontLibrary instance must outlive
/// any FontFaces created with it.
class FontLibrary : public std::enable_shared_from_this<FontLibrary> {
public:
    FontLibrary() = default;
    virtual ~FontLibrary() = default;

    // non-copyable
    FontLibrary(const FontLibrary&) = delete;
    FontLibrary& operator =(const FontLibrary&) = delete;

    static FontLibraryPtr create_instance();
    static FontLibraryPtr default_instance();

    FontFacePtr create_font_face();
};


} // namespace xci::text

#endif // XCI_TEXT_FONTLIBRARY_H
