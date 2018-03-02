// FontLibrary.cpp created on 2018-03-01, part of XCI toolkit

#include <xci/text/FontLibrary.h>

#include <xci/log/Logger.h>
using namespace xci::log;

namespace xci {
namespace text {


FontLibrary::FontLibrary()
{
    auto err = FT_Init_FreeType(&library);
    if (err) {
        log_error("FT_Init_FreeType: {}", err);
        abort();
    }
}


FontLibrary::~FontLibrary()
{
    auto err = FT_Done_FreeType(library);
    if (err) {
        log_error("FT_Done_FreeType: {}", err);
        return;
    }
}


std::shared_ptr<FontLibrary> FontLibrary::get_default_instance()
{
    thread_local static auto instance = std::make_shared<FontLibrary>();
    return instance;
}


}} // namespace xci::text
