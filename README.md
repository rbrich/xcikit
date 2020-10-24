xcikit
======
Collection of C++ libraries for drawing 2D graphics, rendering text and more.

- [About](#about)
- [Features](#features)
- [Libraries](#libraries)
    - [xci::widgets](#xciwidgets)
    - [xci::text](#xcitext)
    - [xci::graphics](#xcigraphics)
    - [xci::script](#xciscript)
    - [xci::data](#xcidata)
    - [xci::core](#xcicore)
    - [xci::compat](#xcicompat)
- [Tools](#tools)
- [How to build](#how-to-build)
    - [Using build script](#using-build-script)
    - [Manual build with CMake](#manual-build-with-cmake)
    - [Porting to Windows](#porting-to-windows)
- [How to use in client program](#how-to-use-in-client-program)
    - [Linking with client using only CMake](#linking-with-client-using-only-cmake)
    - [Linking with client using CMake and Conan](#linking-with-client-using-cmake-and-conan)


About
-----

Xcikit contains basic elements needed for creating 2D graphical applications.
The focus is on text rendering and closely related UI rendering. Additional
features are a custom scripting language and data serialization.

With xcikit you can:

- render a few paragraphs of text,
- style some parts of the text differently (colored highlights),
- respond to mouse hover, click on the highlighted parts (spans),
- create basic UI with buttons, checkboxes, combo-boxes,
- support scripting, provide sandboxed API for user scripts.

This should be enough for a program to render:

- 2D sprites,
- menu,
- settings screen,
- dialogs.

The library uses GLFW and Vulkan for graphics.


Features
--------

Note that xcikit is still under development and mostly experimental.
There is no stable API. There are no releases. The features below are partially planned,
but already implemented to some degree.

The planned features:

- GPU oriented 2D graphics
- advanced text rendering
- UI widgets (not meant to replace Qt, but should be good enough for indie games)
- data file management (VFS)
- custom scripting language (it wasn't planned originally, but it's fun to design and implement)
- support library (logging, eventloop etc.)

The features are divided into a set of libraries:

- `xci-widgets`
- `xci-text`
- `xci-graphics`
- `xci-script`
- `xci-data`
- `xci-core`

The separation of the code into libraries helps to create layers
and avoid bi-directional dependencies. Basically, each library can depend only
on the libraries listed below it. (TODO: draw a dependency graph)

The top-level `xci-widgets` provides UI components with event processing.
It uses text layout engine `xci-text`, which can also be used separately.

All rendering goes through GLFW and Vulkan based `xci-graphics` library.
Graphics lib can be used separately when neither UI nor text rendering
is needed.

Optional `xci-data` library provides custom text and binary format
for serialization / deserialization of hierarchical data structures.
Think of Json and Google Protobuf, but without the complexity and bloat.

All of the above use `xci-core`, which contains miscellaneous utilities,
This can also be used separately.

There is also header-only `compat` library, which is used to hide
differences between compilers and OS's. It does not implement any
abstraction layer, but just provides what is available elsewhere.

Technologies:

- C++20 as main programming language
- CMake as build system
- Conan as dependency manager

Tested compilers:

- GCC 10 (Debian 11 Bullseye)
- Clang 10 (MacOS via Homebrew)

Any Unix-like OS with C++20 compliant compiler should work. There is no direct Windows support,
but it's possible that the project will compile with some layer of Unix compatibility, e.g. WSL.
Some of the libraries already compile natively, see [Porting to Windows](#porting-to-windows).

Libraries
---------

### xci::widgets

Basic UI elements.

### xci::text

Text rendering and text layout.

### xci::graphics

The basic building blocks for rendering of text and UI elements.

### xci::data

Serialization and deserialization of structured data.

### xci::core

Core utilities. These have little or no dependencies. Mostly just stdlib + OS API.

- `Buffer` (`types.h`) - Owned blob of data, with deleter.
- `FpsCounter` - Tracks delays between frames and computes frame rate.
- `Logger` (`log.h`) - Logging functions.
- `SharedLibrary` - Thin wrapper around dlopen. For plugins.
- `TermCtl` - Colored output for ANSI terminals.
- `Vfs` - Unified reading of regular files and archives. Mount the archive to virtual path
  and read contained files in same fashion as regular files.
- `event.h` - System event loop (abstraction of kqueue / epoll).
- `dispatch.h` - Watch files and notify on changes. Useful for auto-reloading of resource files.
- `file.h` - Read whole files. Path utilities (dirname, basename, ...).
- `format.h` - Formatted strings. Similar to Python's `format()`.
- `geometry.h` - 2D vector, rectangle. Linear algebra.
- `rtti.h` - C++ demangled type names.
- `string.h` - String manipulation. Unicode utilities.
- `sys.h` - A replacement for `std::this_thread::get_id()`, providing the canonical TID.

### xci::compat

Fills gaps between different systems and compilers.

- `bit.h` - C++20 `bit_cast` backport (+ custom `bit_read`)
- `dl.h` - `dlopen` etc. for Windows
- `endian.h` - Linux-like macros provided for MacOS
- `macros.h` - `FALLTHROUGH`, `UNREACHABLE`, `UNUSED`
- `unistd.h` - Minimal Unix compatibility header for Windows


Tools
-----

Included are some tools build on top of the libraries.
Check them on separate pages:
 
- [XCI Tools](tools/README.md)
    - [Find File (ff)](tools/find_file/README.md) - `find` alternative


How to build
------------

Build system (required):
- CMake (eg. `apt-get install cmake`)

Package manager (optional):
- Conan (eg. `pip3 install conan`)

Dependencies (required):
- PEGTL (xci-core)
- FreeType (xci-text)
- GLFW, Vulkan (xci-graphics)
- [glslc](https://github.com/google/shaderc) or
  [glslangValidator](https://github.com/KhronosGroup/glslang) (xci-graphics)

Dependencies (optional):
- libzip (XCI_WITH_ZIP)
- [Catch2](https://github.com/catchorg/Catch2) for tests
- [Google Benchmark](https://github.com/google/benchmark)

Installing optional dependencies:
- Debian: `apt-get install libzip-dev`
- macOS (Homebrew): `brew install libzip`


### Using build script

This should be run once to download missing files etc.:

    ./bootstrap.sh

The complete build process is handled by build script:

    ./build.sh

When finished, you'll find the temporary build files in `build/`
and installation artifacts in `artifacts/`.

Both scripts are incremental, so it's safe to run them repeatably.
They do only the required work and re-use what was done previously.


### Manual build with CMake

Detailed build steps (these are examples, adjust parameters as needed):

    # Configure Conan remotes, download additional assets
    ./bootstrap.sh

    # Prepare build directory
    mkdir build && cd build

    # Install dependencies using Conan.
    conan install .. --build missing -s compiler=clang

    # Configure
    cmake .. -G Ninja -DCMAKE_INSTALL_PREFIX=~/sdk/xcikit

    # Adjust CMake configuration
    ccmake ..

    # Build
    cmake --build .

    # Run unit tests
    cmake --build . --target test

    # Install (default prefix is /usr/local)
    cmake --build . --target install


### Porting to Windows

The project is not yet fully ported to Windows. But the essential parts should
already work:

- dependencies via Conan
- build scripts (using git-bash)
- CMake configuration and build
- core and script libraries + demos

How to build:

0) Enable [Developer mode](https://docs.microsoft.com/en-us/windows/uwp/get-started/enable-your-device-for-development)
   (required for ability to create symlinks)

1) Install build tools via their Windows installers: Git, CMake, Conan
   (`git`, `cmake` and `conan` commands should now work in `cmd.exe`)

2) Open *Git Bash* and run `./bootstrap.sh`

3) Still in *Git Bash*, run `./build.sh -D XCI_GRAPHICS=0`

4) There might be errors. These are why the title says "Porting", not "How to build"...


How to use in client program
----------------------------

### Linking with client using only CMake

Build and install XCI libraries (see "How to build" above),
then use installed `xcikitConfig.cmake` in your project's
`CMakeLists.txt`:

    cmake_minimum_required(VERSION 3.7)
    project(example CXX)

    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

    find_package(xcikit REQUIRED)

    add_executable(example src/main.cpp)
    target_link_libraries(example xci-widgets)

In the case xcikit was installed into non-standard location,
for example `~/sdk/xcikit`, you need to setup `CMAKE_PREFIX_PATH` appropriately:

    cmake -DCMAKE_PREFIX_PATH="~/sdk/xcikit" ..


### Linking with client using CMake and Conan

Add xcikit as dependency to `conanfile.txt`:

    [requires]
    xcikit/0.1@rbrich/stable

    [generators]
    cmake_paths

Then include generated `conan_paths.cmake` from project's `CMakeLists.txt`:

    if (EXISTS ${CMAKE_BINARY_DIR}/conan_paths.cmake)
        include(${CMAKE_BINARY_DIR}/conan_paths.cmake)
    endif()

Now find `xcikit` in usual way:

    find_package(xcikit REQUIRED)

Optionally, include XCI goodies:

    include(XciBuildOptions)

Link with the libraries:

    target_link_libraries(example xcikit::xci-text xcikit::xci-graphics)
