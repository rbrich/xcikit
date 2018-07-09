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

Plan
----

Long-term goal:

- advanced text rendering
- not-so-advanced UI widgets
- basic UI logic

These goals should be achieved by separate libraries:

- `xci-util`
- `xci-graphics`
- `xci-text`
- `xci-widgets`

The top-level `xci-widgets` provides UI components with event processing.
It uses text layout engine `xci-text`, which can also be used separately.

All rendering goes through `xci-graphics` library, which provides
adapters for different frameworks and APIs, including low-level OpenGL.
Graphics lib can also be used separately when neither UI nor text rendering is needed.

All of the above use `xci-util`, which contains miscellaneous utilities,
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
- GLFW (WITH_OPENGL)
- SFML (WITH_SFML)
- Panda3D (WATH_PANDA)
- Catch2 (for tests)

Build steps:

    mkdir build && cd build
    
    # Optionally, regenerate glad bindings
    ../bootstrap.sh

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
