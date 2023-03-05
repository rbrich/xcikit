Find File (ff)
==============

A find-like tool using Hyperscan for regex matching and threads for fast directory walk.

![ff (screenshot)](https://user-images.githubusercontent.com/6216152/95231266-39a57180-0803-11eb-90a2-f8de1bf49416.png)

Inspired by [fd](https://github.com/sharkdp/fd).

Implementation:
- fast file tree walk using `fdopendir(3)`, `openat(2)`
- custom threadpool with simple locking queue
- entries sorted lexically, but output from threads is interleaved (no thread output buffers)
- no `stat(2)` by default, dirs are detected using `O_DIRECTORY`

Default ignored files and directories:
- special paths like `/mnt`, `/dev`, `/proc` are not searched by default
- to search in them, either add them explicitly to searched paths or use `-S, --search-in-special-dirs`
- for example, `ff tty /` skips `/dev` while `ff tty / /dev` doesn't skip it
- the list of ignored directories is presented in `--help`, under `--search-in-special-dirs` option

Single device
- allows skipping any mounted directories that are found during directory walk
- this costs additional `stat(2)` call per directory (usually unnoticeable)

Long listing
- `ff -l` show file attributes
- `ff -L` is a shortcut for output similar to `ls -lh`
- suggested shell alias: `alias lf='ff -L'`

Use cases
---------

### Count file sizes

Use `--stats` in combination with `-l` or `-t<TYPE>` (both imply `stat(2)` calls) to print sum of file sizes.

Example:
- `-s` is `--stats`
- `-tf` filters regular files and also enables size statistics
- `-e 'cpp|h'` filters C++ sources
- no matching pattern is used, so we need `--` before the list of paths

<img width="512" alt="ff_stats" src="https://user-images.githubusercontent.com/6216152/107156217-5b636080-697d-11eb-9836-8ed936fd1b2a.png">

### Browse binary executable files

Use `--xmagic` to filter ELF/Mach-O files. These are executables, libraries, .o files.

Useful inside installation directory (build artifacts) to skip data files and focus on executable code.

Example:
- `-j1` for ordered output (otherwise, it's interleaved from multiple threads).
- `-l` for detailed listing (file attributes)

<img width="544" alt="ff_xmagic" src="https://user-images.githubusercontent.com/6216152/207453664-05c1df89-bd0c-4de7-8080-72a152d0c3bb.png">

Development
-----------

Possible features to be added:
- filter files by attributes, e.g. file size (use case: search for large files)

Not planned:
- interpretation of `.gitignore`
   - This would add a ton of complexity with questionable profit. If you want to hide
     build products from search tools like `ff`, then consider building into a hidden directory
     like `.build`.


Build
-----

Build only ff tool `ff` tool + necessary libs (run from repo root, not tools subdirectory):

```bash
./build.sh only-ff
```
