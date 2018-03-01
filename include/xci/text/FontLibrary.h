// FontLibrary.h created on 2018-03-01, part of XCI toolkit

#ifndef XCI_TEXT_FONTLIBRARY_H
#define XCI_TEXT_FONTLIBRARY_H

#include <ft2build.h>
#include FT_FREETYPE_H

namespace xci {
namespace text {


// FontLibrary is a factory for FontFace objects.
//
// By default, it has one instance per thread, unless you construct
// new instances explicitly. In that case, it's up to you how you manage
// their lifetime. But keep in mind that the FontLibrary instance must outlive
// any FontFaces created with it.
//
// See also the FT_Library documentation.

class FontLibrary {
public:
    FontLibrary();
    ~FontLibrary();

    static FontLibrary& get_default_instance();

    // non-copyable
    FontLibrary(const FontLibrary&) = delete;
    FontLibrary& operator =(const FontLibrary&) = delete;

    FT_Library& get() { return library; }

private:
    FT_Library library;
};


}} // namespace xci::text

#endif // XCI_TEXT_FONTLIBRARY_H
