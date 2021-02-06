from conans import ConanFile


class IncbinConan(ConanFile):
    name = "incbin"
    description = "Include binary files in C/C++"
    homepage = "https://github.com/graphitemaster/incbin.git"
    version = "20200420"
    license = "Unlicense"
    scm = {
        "type": "git",
        "url": homepage,
        "revision": "8cefe46d5380bf5ae4b4d87832d811f6692aae44",
    }

    # package info
    url = "https://github.com/rbrich/xcikit/tree/master/conan/incbin"
    author = "Radek Brich"
    generators = "cmake"

    def package(self):
        self.copy("incbin.h", dst="include")
