XCI Tools
=========

DAR Packer (pack_assets.py)
---------------------------

Concatenates multiple files into single archive, similarly to what tar does.
The file index is at the end of the file, so it's easy to append more files
to the archive. Stores only the file name and content. No attributes.

Example usage:

    find assets -type f > asset_list.txt
    tools/pack_assets.py assets.dar asset_list.txt
