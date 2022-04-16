// FontLibrary.h created on 2018-03-01 as part of xcikit project
// https://github.com/rbrich/xcikit
//
// Copyright 2018–2021 Radek Brich
// Licensed under the Apache License, Version 2.0 (see LICENSE file)

#ifndef XCI_TEXT_FONTLIBRARY_H
#define XCI_TEXT_FONTLIBRARY_H

#include <memory>
#include <vector>
#include <string>

#include <xci/core/error.h>
#include <xci/core/mixin.h>

// Forward decls from <ft2build.h>
// (to avoid external dependency on Freetype)
typedef struct FT_LibraryRec_  *FT_Library;  // NOLINT(modernize-use-using)


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
class FontLibrary : public std::enable_shared_from_this<FontLibrary>, private core::NonCopyable {
public:
    FontLibrary();
    ~FontLibrary();

    static FontLibraryPtr create_instance();
    static FontLibraryPtr default_instance();

    FontFacePtr create_font_face();

    FT_Library ft_library() const { return m_ft_library; }

private:
    FT_Library m_ft_library = nullptr;
};


} // namespace xci::text

#endif // XCI_TEXT_FONTLIBRARY_H
