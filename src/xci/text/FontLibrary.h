// FontLibrary.h created on 2018-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

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
