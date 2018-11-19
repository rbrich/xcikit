from conans import ConanFile


class IncbinConan(ConanFile):
    name = "incbin"
    description = "Include binary files in C/C++"
    homepage = "https://github.com/graphitemaster/incbin.git"
    version = "20180413"
    license = "Unlicense"
    _source_commit = "c61cae60e3d47fd3d59e937693c0c4787dcc55ea"

    # package info
    url = "https://github.com/rbrich/xcikit/tree/master/conan/incbin"
    author = "Radek Brich"
    generators = "cmake"

    def source(self):
        self.run("git clone {.homepage}".format(self))
        self.run("cd incbin && git checkout {._source_commit}".format(self))

    def package(self):
        self.copy("incbin.h", dst="include", src="incbin")
