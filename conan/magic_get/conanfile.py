from conans import ConanFile, tools


class MagicGetConan(ConanFile):
    name = "magic_get"
    version = "1.0.0"
    description = "Precise and Flat Reflection (ex Magic Get, ex PODs Flat Reflection)"
    homepage = "https://github.com/apolukhin/magic_get"
    license = "Boost-1.0"

    # package info
    url = "https://github.com/rbrich/xcikit/tree/master/conan/magic_get"
    author = "Radek Brich"
    generators = "cmake"

    def source(self):
        tools.get(f"{self.homepage}/archive/{self.version}.tar.gz")

    def package(self):
        self.copy("*", src=f"{self.name}-{self.version}/include", dst="include")
