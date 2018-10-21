Abseil Conan Recipe
===================

Based on bincrafters recipe:
- [bincrafters/conan-abseil](https://github.com/bincrafters/conan-abseil)
- version: 7a9b8e67912360b460c77b2f152c0817d3afd2c6

Even though we build Abseil with CMake, it does not have install target
nor CMake config with exported targets. We have to collect libraries manually
and write custom CMake Find script with imported targets to allow smooth use in CMake projects.

See: 
- [Allow Abseil to be installed and used with find_package()](https://github.com/abseil/abseil-cpp/issues/111)
