import os

from conans import ConanFile, CMake, tools


class XcikitTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = ("cmake_find_package_multi", "cmake_paths")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.install()

    def imports(self):
        self.copy("*.dylib*", dst="lib", src="lib")
        self.copy('*.so*', dst='lib', src='lib')

    def test(self):
        if not tools.cross_building(self.settings):
            self.run("./example")
