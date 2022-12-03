/**
 * widechar_width.h - split from original widechar_width.h, generated on 2020-08-04.
 * See https://github.com/ridiculousfish/widecharwidth/
 *
 * SHA1 file hashes:
 *  UnicodeData.txt:     95d6515be1580b23ff9a683b5cb5c154b2ae7381
 *  EastAsianWidth.txt:  5464582b07faa40b45e1450e9f580e9af2c5a9d3
 *  emoji-data.txt:      b220903e472dbc2b57cd4d74904a540b0e0619ee
 */

#ifndef WIDECHAR_WIDTH_H
#define WIDECHAR_WIDTH_H

#include <cstddef>

namespace wcw {

/* Special width values */
enum {
    widechar_nonprint = -1,     // The character is not printable.
    widechar_combining = -2,    // The character is a zero-width combiner.
    widechar_ambiguous = -3,    // The character is East-Asian ambiguous width.
    widechar_private_use = -4,  // The character is for private use.
    widechar_unassigned = -5,   // The character is unassigned.
    widechar_widened_in_9 = -6  // Width is 1 in Unicode 8, 2 in Unicode 9+.
};

/* Return the width of character c, or a special negative value. */
int widechar_wcwidth(char32_t c);

} // namespace
#endif // WIDECHAR_WIDTH_H
