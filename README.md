XCI Toolkit
===========
Render Text & User Interface

![Font Texture](http://xci.cz/toolkit/img/xci-text.png)

*The text on the right side was rendered from the texture on the left side*


Why?
----

Text and UI rendering seems to be somewhat lacking in many frameworks
for 2D/3D graphics. I basically want to be able to:

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


Plan
----

Long-term goal:

- advanced text rendering,
- not-so-advanced UI widgets,
- basic UI logic

These goals should be achieved by separate libraries:

- `xci-ui`
- `xci-widgets`
- `xci-text`

When only `xci-text` is to be used, no UI widgets need to be even compiled.
For just rendering widgets (ie. immediate mode), only `xci-widgets` + `xci-text`
are to be used. The top-level `xci-ui` will provide UI with input processing,
events, callbacks...

All rendering should go through `xci-graphics` library, which provides
adapters for different frameworks and APIs, including low-level OpenGL.

Technologies:

- C++11
- CMake
- Python bindings - Cython?


Roadmap
-------

- [x] Basic text rendering
- [x] Text layout with styled spans
- [x] Raw OpenGL
- [x] Draw a button
- [ ] Hover, click events
- [ ] ...


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
- GLFW (WITH_OPENGL)
- SFML (WITH_SFML)
- Panda3D (WATH_PANDA)
- Catch2 (for tests)

Build steps:

    mkdir build && cd build
    
    # Optionally, install dependencies using Conan.
    # Otherwise, they will be looked up in default system locations
    # or specified paths (see in cmake-modules).
    ../.conan/create-local.sh
    conan install .. [-s compiler=clang]
    
    # Configure
    cmake ..
    
    # Optionally, adjust configuration
    ccmake ..
    
    # Build
    make
