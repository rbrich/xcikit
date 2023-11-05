DAR archive extractor (dar)
===========================

A tool for extracting asset archives. Supports DAR, WAD, ZIP formats.

To create an archive, see [pack_assets.py](../pack_assets.py).

WAD format
----------

This is DOOM 1 file format. Its directory is different from other formats:
* "lump" name limited to 8 chars
* hierarchy emulated by special empty lumps (*START/*END)
* lump names may repeat and order is important

To work around these specifics, the extractor creates an index file (`.wad`)
and renames some files so they don't collide. The mapping of lump names to file names
is preserved in `.wad` ("<lump name>\t<file path>"). The first line of `.wad` preserves
the type of WAD file: "IWAD" or "PWAD".
