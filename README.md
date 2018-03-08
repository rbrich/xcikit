XCI Toolkit
===========
Render Text & User Interface

![Font Texture](http://xci.cz/toolkit/img/xci-text.png)
*The text on right side was rendered from the texture on left side*


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

- Panda3D, OpenSceneGraph
- SFML, SDL
- raw OpenGL


Competitors
-----------

TODO: reference libraries like CeGUI


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
- [ ] Text layout with styled spans
- [ ] Raw OpenGL
- [ ] Draw a button
- [ ] Hover, click events
- [ ] ...
