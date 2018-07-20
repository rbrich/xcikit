Text Terminal
=============

The XCI Widgets library contains TextTerminal widget, which can be used to display monospaced
text with the usual attributes (colors, bold mode, blinking etc.).

This document describes how this widget stores the data in memory.


Line Buffer
-----------

Let's start with the simplest case - linear text being appended at the end (eg. cat log file).
The text is first split into lines. There are two causes of line break:

- newline character (0x10)
- line overflows the width of terminal (soft break)

In both cases, new Line is added into the Buffer. In first case, the split is made after newline
character (which is preserved at the end of previous line). In the second case, there is no
newline character, so previous line is not ended with it - this is a soft break.

When reconstructing the original text, Lines from the Buffer needs to be just concatenated together.


Character Encoding
------------------

The terminal uses standard UTF-8 encoding for text in the Buffer. This has advantages:

- saves space (in comparison to array of 4 byte code points)
- text input doesn't need to be transcoded

But there are also disadvantages. Most importantly, the text has to be decoded before
it can be displayed as a sequence of font glyphs.


Encoding Attributes
-------------------

The buffer could be a simple matrix of (codepoint, attributes) pairs. After some consideration,
I chose a more compact representation using UTF-8 strings interleaved with specially encoded
attributes.

The attributes rarely change with each character, more commonly, they are changed only at some
points - eg. whole worlds in same color. To save memory, the attributes can be put into text string
only when some change occurs. To make it possible to draw each line separately, attributes have
to be also repeated at the start of each line.

For example:

    {bold red} Highlighted world {normal white} and the rest of text.
    {normal white} Next line.

Now, how to encode the attributes in UTF-8 string? Fortunately, not all byte sequences are valid
UTF-8 strings. We'll use the invalid sequences for our attributes.

Introducers not valid in UTF-8:

- `11000000` (192, 0xc0), `11000001` (193, 0xc1):
  leading bytes for 2-byte sequences which lead to *overlong* encoding of 7-bit ASCII,
  which is disallowed in valid UTF-8

- `11110101` (245, 0xf5), `11110110` (246, 0xf6), `11110111` (247, 0xf7):
  leading bytes for 4-byte sequences out of UTF-8 range (which is defined up to `100001111111111111111`)

- `11111000` (248, 0xf8), `11111001` (249, 0xf9), `11111010` (250, 0xfa), `11111011` (251, 0xfb):
  leading bytes for 5-byte sequences (not defined in UTF-8)

- `11111100` (252, 0xfc), `11111101` (253, 0xfd):
  leading bytes for 6-byte sequences (not defined in UTF-8)
  
- `11111110` (254, 0xfe), `11111111` (255, 0xff):
  not defined in UTF-8 (these could introduce 7 and 8 byte sequences)

So we have plenty to chose from. While using only 6 significant bits for continuation bytes, we could
make the resulting strings look almost like valid UTF-8. But that would make decoding a little harder.
Let's use just the introducer to start our custom attribute sequence, which will be completely invalid
in UTF-8 encoding. 

### The Attributes

What do we need to encode? Useful attributes from ECMA-48 standard
(controlled via SGR - Select Graphic Rendition):

- Mode: normal, bold, faint (dim), conceal, italic, blink (slow/rapid), reverse video,
        underline, overlined, crossed-out, framed, encircled
- Color: foreground, background (3-bit, 8-bit, 24-bit)
- Font: primary, alternative 1-9

If we want to support (almost) everything, we'll need:

- 8 bits for modes:
  - 2 bits: normal, bold, faint, conceal (4 values)
  - 1 bit: reverse video
  - 1 bit: italic
  - 3 bits: no decoration, underline, overlined, crossed-out, framed, encircled (6 values + 2 reserved)
  - 1 bit: blink (only slow)

- 2x 8 bits for 256 color palette (fg, bg) - this includes basic 3-bit colors
- 2x 24 bits for full RGB (fg, bg)
- alternative fonts - 1-8 bits (could be useful for icon fonts)

Possible encoding:

- set mode: 0xc0 + 8bits
- set 16-colors fg + bg: 0xc1 + 8bits
- set RGB fg: 0xf5 + 24bits
- set RGB bg: 0xf6 + 24bits
