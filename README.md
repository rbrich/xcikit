XCI Toolkit
===========
Toolkit for rendering text and simple user interface with OpenGL.

![Font Texture](http://xci.cz/toolkit/img/xci-text.png)

*The text on the right side was rendered from the texture on the left side*


About
-----

Text and UI rendering seems to be somewhat lacking in many graphics frameworks
for 2D/3D games. I basically want to be able to:

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
- SFML, SDL
- Panda3D, OpenSceneGraph


Research 
--------

OpenGL GUI toolkits:

- [CEGUI](http://cegui.org.uk/)
- [Dear ImGui](https://github.com/ocornut/imgui) - focuses on debug UIs
- [Nuklear](https://github.com/vurtun/nuklear) - C
- [Turbo Badger](https://github.com/fruxo/turbobadger)

General GUI toolkits:

- [Qt](https://www.qt.io/)
- [GTK+](https://developer.gnome.org/gtk3/stable/index.html)

Low-level graphics:

- [mango fun framework](https://github.com/t0rakka/mango)


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

- C++14
- CMake
- Python bindings - Cython?


Roadmap
-------

- [x] Basic libraries as described above
- [ ] More widgets: context menu, combo box
- [ ] Floating windows
- [ ] Integrate with Panda3D
- [ ] Screensaver support
- [ ] New roadmap...


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
- SFML (XCI_WITH_SFML)
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
    conan install .. -s compiler=clang

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
