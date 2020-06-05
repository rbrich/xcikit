Catch2 - Conan package
======================

Custom build.


## Why?

Official Catch2 package moved to ConanCenter, unfortunately.
ConanCenter packages currently don't distribute CMake configs,
so they are unusable with `cmake-paths` generator.

Some of the issues regarding this "enhancement":
- [[boost] System-wide FindBoost.cmake used for builds #93](https://github.com/conan-io/conan-center-index/issues/93)
- [[RFC] Problems with deleting config.cmake and .pc files #209](https://github.com/conan-io/conan-center-index/issues/209)
- [[question] Mimicking CMake Find Modules #419](https://github.com/conan-io/conan-center-index/issues/419)
- [[question] fftw/all #488](https://github.com/conan-io/conan-center-index/issues/488)
- [Why is the cmake configuration in Boost deleted when packaging ? #1818](https://github.com/conan-io/conan-center-index/issues/1818)


## How to build

Use upstream recipe:

    VERSION=2.12.2
    git clone https://github.com/catchorg/Catch2.git
    git checkout v${VERSION}
    conan create . Catch2/${VERSION}@rbrich/stable
