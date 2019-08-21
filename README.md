XCI Toolkit
===========
Collection of C++ libraries focused on drawing 2D graphics and rendering text.

![Font Texture](http://xci.cz/toolkit/img/xci-text.png)

*The text on the right side was rendered from the texture on the left side*

- [About](#about)
- [Features](#features)
- [Contents of the libraries](#contents-of-the-libraries)
    - [xci::widgets](#xciwidgets)
    - [xci::text](#xcitext)
    - [xci::graphics](#xcigraphics)
    - [xci::data](#xcidata)
    - [xci::core](#xcicore)
    - [xci::compat](#xcicompat)
- [How to build](#how-to-build)
    - [Using build script](#using-build-script)
    - [Manual build with CMake](#manual-build-with-cmake)
- [How to use in client program](#how-to-use-in-client-program)
    - [Linking with client using only CMake](#linking-with-client-using-only-cmake)
    - [Linking with client using CMake and Conan](#linking-with-client-using-cmake-and-conan)


About
-----

XCI Toolkit contains basic elements needed for creating simple graphical demos
and games. The focus is on text rendering and closely related UI rendering.

With this toolkit, it should be easy to:

- render a few paragraphs of text,
- style some parts of the text differently (colored highlights),
- respond to mouse hover, click on the highlighted parts (spans),
- add some buttons, checkboxes, combo-boxes.

This should be enough for a game to render:

- menu,
- settings screen,
- dialogs.

The library should integrate well with:

- generic OpenGL
- 2D/3D graphics engines (eg. SDL, Magnum, OGRE)


Features
--------

Note that the toolkit is still under development and mostly experimental.
There is no stable API. There are no releases. The features below are partially planned, but already
implemented to some degree. In short, this is one-man show, I did not announce it
anywhere, so nobody probably even knows about it. The development is expected
to be slow but steady.

The target features:

- advanced text rendering
- some UI widgets (not meant to replace Qt)
- GPU oriented 2D graphics
- data file management (VFS)
- support library (logging, eventloop etc.)

These features are implemented by a set of libraries:

- `xci-widgets`
- `xci-text`
- `xci-graphics`
- `xci-data`
- `xci-core`

The separation of the code into libraries also helps to create layers
and avoid bi-directional dependencies. As listed above, each library
depends only on the libraries listed after it. (But nothing depends
on `xci-data` - that one is completely optional.)

The top-level `xci-widgets` provides UI components with event processing.
It uses text layout engine `xci-text`, which can also be used separately.

All rendering goes through `xci-graphics` library, which provides
adapters for different frameworks and APIs, including low-level OpenGL.
Graphics lib can also be used separately when neither UI nor text rendering
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

- C++17 as main programming language
- CMake as build system
- Conan as dependency manager

Tested compilers:

- GCC 8.3 (Debian Buster)
- AppleClang 11 (Travis CI)

Any Unix-like OS with C++17 compliant compiler should work. There is no direct Windows support,
but it's possible that the project will compile with some layer of Unix compatibility, e.g. WSL. 


Contents of the libraries
-------------------------

### xci::widgets

Basic UI elements.

### xci::text

Text rendering and text layout.

### xci::graphics

The basic building blocks for rendering of text and UI elemenents.

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
- `endian.h` - Linux-like macros provided for MacOS
- `macros.h` - C++17 `[[fallthrough]]` missing in GCC 6.3
- `string_view.h` - C++17 `string_view` missing in GCC 6.3, but it has `experimental` impl
- `utility.h` - C++17 `byte` missing in GCC 6.3


How to build
------------

Build system (required):
- CMake (eg. `apt-get install cmake`)

Package manager (optional):
- Conan (eg. `pip3 install conan`)

Dependencies (required):
- FreeType
- PEGTL

Dependencies (optional):
- libzip (XCI_WITH_ZIP)
- GLFW (XCI_WITH_OPENGL)
- Magnum (XCI_WITH_MAGNUM)
- Panda3D (XCI_WITH_PANDA)
- Catch2 (for tests)
- Google Benchmark (for benchmarks)

Installing dependencies:
- Debian: `apt-get install libbenchmark-dev`
- macOS (Homebrew): `brew install libzip google-benchmark`


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

    target_link_libraries(example xcikit::xci-text xcikit::xci-graphics-opengl)
