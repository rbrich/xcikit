import os

from conans import ConanFile, CMake, tools


class XcikitTestConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = ("cmake_paths", "cmake_find_package")

    def build(self):
        cmake = CMake(self)
        # The current dir is "test_package/build/<build_id>"
        # CMakeLists.txt is in "test_package"
        cmake.definitions["CMAKE_INSTALL_PREFIX"] = os.getcwd()
        cmake.configure()
        cmake.build(target="install")

    def imports(self):
        self.copy("*.dylib*", dst="lib", src="lib")
        self.copy('*.so*', dst='lib', src='lib')

    def test(self):
        if not tools.cross_building(self.settings):
            os.chdir("bin")
            self.run("./example")
