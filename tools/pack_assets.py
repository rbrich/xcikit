#!/usr/bin/env python3
"""
Data Archive tool

Creates DAR archives.
See: docs/data/archive_format.adoc
"""

import sys
import os
import struct
import argparse
from contextlib import contextmanager


class Archive:

    ID = b'dar1'

    def __init__(self, archive_file, quiet=False):
        self._f = open(archive_file, 'wb')
        self._index = []  # tuples: (path, offset, size, metadata_size)
        self._index_offset = 0
        self._index_size = 8
        self._write_header()
        if quiet:
            self._info = lambda _: None

    def close(self):
        # write index of files at end of archive
        self._write_index()
        # update header (INDEX_OFFSET)
        self._f.seek(0)
        self._write_header()
        self._info("Written %d files." % len(self._index))

    def add_file(self, src_path, archive_path):
        offset = self._f.tell()
        size = 0
        metadata_size = 0
        with open(src_path, 'rb') as f:
            data = f.read()
            while len(data):
                self._f.write(data)
                size += len(data)
                data = f.read()
        path = archive_path.encode('utf8')
        self._index.append((path, offset, size, metadata_size))
        self._index_size += 16 + len(path)
        self._info("+ %6d  %s" % (size, archive_path))

    def _write_header(self):
        assert self._f.write(self.ID) == 4, "write ID"
        index_offset = struct.pack('!L', self._index_offset)
        assert self._f.write(index_offset) == 4, "write INDEX_OFFSET"

    def _write_index(self):
        self._index_offset = self._f.tell()
        index_size = struct.pack('!L', self._index_size)
        assert self._f.write(index_size) == 4, "write INDEX_SIZE"
        num_entries = struct.pack('!L', len(self._index))
        assert self._f.write(num_entries) == 4, "write NUMBER_OF_ENTRIES"
        for entry in self._index:
            self._write_index_entry(*entry)  # INDEX_ENTRY

    def _write_index_entry(self, path, offset, size, metadata_size):
        offset = struct.pack('!L', offset)
        assert self._f.write(offset) == 4, "write CONTENT_OFFSET"
        size = struct.pack('!L', size)
        assert self._f.write(size) == 4, "write CONTENT_SIZE"
        metadata_size = struct.pack('!L', metadata_size)
        assert self._f.write(metadata_size) == 4, "write METADATA_SIZE"
        assert self._f.write(b"--") == 2, "write ENCODING" # = plain data
        path_size = struct.pack('!H', len(path))
        assert self._f.write(path_size) == 2, "write NAME_SIZE"
        self._f.write(path)  # NAME

    def _info(self, msg):
        print(msg)


@contextmanager
def open_archive(*args, **kwargs):
    archive = Archive(*args, **kwargs)
    try:
        yield archive
    finally:
        archive.close()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('archive_file', type=str,
                    help="Name of archive to be created")
    ap.add_argument('file', type=str, nargs='*',
                    help="Name of archive to be created")
    ap.add_argument("--list-file", type=str,
                    help="Name of file containing list of files to be archived")
    ap.add_argument("-q", "--quiet", action="store_true",
                    help="Do not show progress")
    args = ap.parse_args()

    files = args.file
    src_dir = os.getcwd()
    if args.list_file:
        src_dir = os.path.dirname(args.list_file)
        with open(args.list_file, 'r', encoding='utf8') as f:
            files += [ln.strip() for ln in f.readlines()]

    with open_archive(args.archive_file, args.quiet) as archive:
        for path in files:
            src_path = os.path.join(src_dir, path)
            archive.add_file(src_path, path)


if __name__ == '__main__':
    main()
