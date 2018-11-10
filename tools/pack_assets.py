#!/usr/bin/env python3
# Data Archive tool
"""

Archive File Format
===================
Non-compressed archive with index at the end.
All fields are in big-endian (network order).
All offsets are relative to start of file (simple seek will always work).
File contents are simply concatenated (no padding is inserted).
Inspired by MAR (https://wiki.mozilla.org/Software_Update:MAR)

ARCHIVE:
- HEADER
- FILE [NUMBER_OF_ENTRIES]
- INDEX

HEADER:
- ID = "dar\n" (4 bytes)
- INDEX_OFFSET (4 bytes)

FILE:
- CONTENT (CONTENT_SIZE bytes)

INDEX:
- NUMBER_OF_ENTRIES (4 bytes)
- INDEX_ENTRY [NUMBER_OF_ENTRIES]

INDEX_ENTRY:
- CONTENT_OFFSET (4 bytes)
- CONTENT_SIZE (4 bytes)
- NAME_SIZE (2 bytes)
- NAME (NAME_SIZE bytes)

"""

import sys
import os
import struct


class Archive:

    ID = b'dar\n'

    def __init__(self, archive_file):
        self._f = open(archive_file, 'wb')
        self._index = []  # tuples: path, offset, size
        self._index_offset = 0
        self._write_header()

    def __del__(self):
        # write index of files at end of archive
        self._write_index()
        # update header (index offset)
        self._f.seek(0)
        self._write_header()
        print("Written %d files." % len(self._index))

    def add_file(self, src_path, archive_path):
        offset = self._f.tell()
        size = 0
        with open(src_path, 'rb') as f:
            data = f.read()
            while len(data):
                self._f.write(data)
                size += len(data)
                data = f.read()
        self._index.append((archive_path, offset, size))
        print("+ %6d  %s" % (size, archive_path))

    def _write_header(self):
        assert self._f.write(self.ID) == 4, "write ID"
        index_offset = struct.pack('!L', self._index_offset)
        assert self._f.write(index_offset) == 4, "write INDEX_OFFSET"

    def _write_index(self):
        self._index_offset = self._f.tell()
        num_entries = struct.pack('!L', len(self._index))
        assert self._f.write(num_entries) == 4, "write NUMBER_OF_ENTRIES"
        for entry in self._index:
            self._write_index_entry(*entry)  # INDEX_ENTRY

    def _write_index_entry(self, path, offset, size):
        offset = struct.pack('!L', offset)
        assert self._f.write(offset) == 4, "write CONTENT_OFFSET"
        size = struct.pack('!L', size)
        assert self._f.write(size) == 4, "write CONTENT_SIZE"
        path = path.encode('utf8')
        path_size = struct.pack('!H', len(path))
        assert self._f.write(path_size) == 2, "write NAME_SIZE"
        self._f.write(path)  # NAME


def main():
    if len(sys.argv) != 3:
        print("Usage: {} <archive_file> <list_file>".format(sys.argv[0]))
        sys.exit(1)

    list_file = sys.argv[2]
    with open(list_file, 'r', encoding='utf8') as f:
        lines = [l.strip() for l in f.readlines()]

    archive_file = sys.argv[1]
    archive = Archive(archive_file)
    src_dir = os.path.dirname(list_file)
    for path in lines:
        src_path = os.path.join(src_dir, path)
        archive.add_file(src_path, path)


if __name__ == '__main__':
    main()
