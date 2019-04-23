XCI Toolkit
===========
Toolkit for rendering text and simple user interface with OpenGL.

![Font Texture](http://xci.cz/toolkit/img/xci-text.png)

*The text on the right side was rendered from the texture on the left side*


About
-----

This is a library of basic elements useful for creating simple games and graphical demos.
The focus is on text rendering and closely connected UI rendering.

The library should make it easy to:

- render a few paragraphs of text
- style some parts of the text differently (colored highlights)
- respond to mouse hover, click on the highlighted parts (spans)
- add some buttons, checkboxes, combo-boxes

This should be enough for games to render:

- menu
- settings screen
- dialogs

The library should integrate well with:

- generic OpenGL
- 2D/3D graphics engines (eg. SDL, Magnum, OGRE)


Plan
----

Long-term goal:

- advanced text rendering
- not-so-advanced UI widgets
- basic UI logic

These goals should be achieved by separate libraries:

- `xci-core`
- `xci-graphics`
- `xci-text`
- `xci-widgets`

The top-level `xci-widgets` provides UI components with event processing.
It uses text layout engine `xci-text`, which can also be used separately.

All rendering goes through `xci-graphics` library, which provides
adapters for different frameworks and APIs, including low-level OpenGL.
Graphics lib can also be used separately when neither UI nor text rendering is needed.

All of the above use `xci-core`, which contains miscellaneous utilities,
This can also be used separately. 

Technologies:

- C++17 (partial compatibility with C++11/14 as needed)
- CMake, Conan
- Python bindings - Cython?


Roadmap
-------

- [x] Basic libraries as described above
- [ ] More widgets: context menu, combo box
- [ ] Floating windows
- [x] Integrate with GLFW and one other library
- [ ] Screensaver support
- [ ] New roadmap...


Current Features
----------------

Supported compilers:

- GCC 6.3 (tested with Debian Stretch)
- AppleClang 9.1

C++ standard set to C++17, but only bits of it are used due to GCC 6.3 compatibility
requirement.

### xci::compat

Fills gaps between different systems and compilers.

- `bit.h` - C++20 `bit_cast` backport (+ custom `bit_read`)
- `endian.h` - Linux-like macros provided for MacOS
- `macros.h` - C++17 `[[fallthrough]]` missing in GCC 6.3
- `string_view.h` - C++17 `string_view` missing in GCC 6.3, but it has `experimental` impl
- `utility.h` - C++17 `byte` missing in GCC 6.3

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

### xci::graphics

The basic building blocks for rendering of text and UI elemenents.

### xci::text

Text rendering and text layout.

### xci::widgets

Basic UI elements.


Build
-----

Build system (required):
- CMake (eg. `apt-get install cmake`)

Package manager (optional):
- Conan (eg. `pip3 install conan`)

Dependencies (required):
- FreeType
- PEGTL

Dependencies (optional):
- GLFW (XCI_WITH_OPENGL)
- Magnum (XCI_WITH_MAGNUM)
- Panda3D (XCI_WITH_PANDA)
- Catch2 (for tests)
- Google Benchmark (for benchmarks)

Installing dependencies:
- Debian: `apt-get install libbenchmark-dev`
- macOS (Homebrew): `brew install google-benchmark`

Build steps (these are examples, adjust parameters as needed):

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


Linking with XCI Toolkit (using CMake)
--------------------------------------

Build and install XCI libraries (see Build above),
then use installed `xcikitConfig.cmake` from your project's
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


Linking with XCI Toolkit (using CMake + Conan)
----------------------------------------------

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
