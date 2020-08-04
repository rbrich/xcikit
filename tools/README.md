XCI Tools
=========

Find File (ff)
--------------

A find-like tool using Hyperscan for regex matching.

TODO: screenshot


Fire Script (fire)
------------------

Fire Script compiler and interpreter.
See [Syntax reference](../docs/script/syntax.md).


DAR Packer (pack_assets.py)
---------------------------

Concatenates multiple files into single archive, similarly to what tar does.
The file index is written at the end of the file, so it's easy to append
more files to the archive. Stores only the file name and content. No attributes.

Example usage - pack whole "share" directory:

    (cd share; find shaders fonts -type f > file_list.txt)
    tools/pack_assets.py share.dar share/file_list.txt

DAR means "Data ARchive"
