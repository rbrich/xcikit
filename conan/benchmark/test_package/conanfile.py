from conans import ConanFile, CMake, tools


class GoogleBenchmarkConanTest(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "cmake_paths"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if not tools.cross_building(self.settings):
            self.run("./example")
