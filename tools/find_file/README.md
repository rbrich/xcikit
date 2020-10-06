Find File (ff)
==============

A find-like tool using Hyperscan for regex matching and threads for fast directory walk.

![ff (screenshot)](screenshot.png)

Inspired by [fd](https://github.com/sharkdp/fd).

Implementation:
- fast file tree walk using `fdopendir(3)`, `openat(2)`
- custom threadpool with simple locking queue
- no sorting, no `stat(2)` (dirs are detected using `O_DIRECTORY`)

Default ignored files and directories:
- special paths like `/mnt`, `/dev`, `/proc` are not searched by default
- to search in them, either add them explicitly to searched paths or use `-S, --search-in-special-dirs`
- for example, `ff tty /` skips `/dev` while `ff tty / /dev` doesn't skip it
- the list of ignored directories is presented in `--help`, under `--search-in-special-dirs` option

Single device
- allows skipping any mounted directories that are found during directory walk
- this costs additional `stat(2)` call per directory (usually unnoticeable)

Development
-----------

Possible features to be added:
- sorting would be nice, sometimes
- option to `stat(2)` each found file and show or inspect some details, e.g. file size

Not planned:
- interpretation of `.gitignore`
   - This would add a ton of complexity with questionable profit. If you want to hide
     build products from search tools like `ff`, then consider building into a hidden directory
     like `.build`.
