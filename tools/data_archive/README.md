DAR archive (dar)
=================

A tool for extracting asset archives. Supports DAR, WAD, ZIP formats.
Packing is not supported, but there is a Python script which can do that
(see dar_pack.py).


DAR format
----------

Custom archive format, with support for per-file deflate compression.
Files are stored next to each other, index is attached at the end of archive.

See [archive_format.adoc](../../docs/data/archive_format.adoc).


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


DAR Packer (dar_pack.py)
------------------------

Concatenates multiple files into single archive, similarly to what tar does.
The file index is written at the end of the file, so it's easy to append
more files to the archive. Stores only the file name and content. No attributes.

Example usage - pack whole "share" directory:

    (cd share; find shaders fonts -type f > file_list.txt)
    tools/data_archive/dar_pack.py share.dar share/file_list.txt

