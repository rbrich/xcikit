XCI Tools
=========

DAR Packer (pack_assets.py)
---------------------------

Concatenates multiple files into single archive, similarly to what tar does.
The file index is written at the end of the file, so it's easy to append
more files to the archive. Stores only the file name and content. No attributes.

Example usage - pack whole "share" directory:

    find share -type f > file_list.txt
    tools/pack_assets.py share.dar file_list.txt

DAR means "Data ARchive"
