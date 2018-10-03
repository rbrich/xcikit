from conans import ConanFile


class IncbinConan(ConanFile):
    name = "incbin"
    version = "20180413-c61cae6"
    license = "Unlicense"
    url = "https://github.com/graphitemaster/incbin.git"
    branch = "c61cae60e3d47fd3d59e937693c0c4787dcc55ea"
    description = "Include binary files in C/C++"

    def source(self):
        self.run("git clone {.url}".format(self))
        self.run("cd incbin && git checkout {.branch}".format(self))

    def package(self):
        self.copy("incbin.h", dst="include/incbin", src="incbin")
