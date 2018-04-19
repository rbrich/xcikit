Plugin Demo
===========

Demonstrates how to create hot-swappable shared library.

1. Set `CMAKE_INSTALL_PREFIX` to eg. `~/xcikit` (so you don't need sudo to install)

2. Build and install the demo. This installs *pluggable* library in `~/xcikit/lib`

3. Run `demo_plugin`. It will load the *pluggable* library
   and call it once per second.

4. Modify `pluggable.cpp`, rebuild and install again.

5. The running demo detects the change, reloads library and starts showing
   the modified text.

