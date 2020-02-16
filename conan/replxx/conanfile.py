from conans import ConanFile, CMake, tools


class ReplxxConan(ConanFile):
    name = "replxx"
    description = "A readline and libedit replacement that supports UTF-8, " \
                  "syntax highlighting, hints and Windows and is BSD licensed."
    homepage = "https://github.com/AmokHuginnsson/replxx"
    version = "20190926"
    license = "BSD"
    scm = {
        "type": "git",
        "url": homepage,
        "revision": "c8fbb3af03cd151e935f556237d6617961244095",
    }

    # package info
    url = "https://github.com/rbrich/xcikit/tree/master/conan/replxx"
    author = "Radek Brich"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "cmake"

    def config_options(self):
        if self.settings.os == 'Windows':
            del self.options.fPIC

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
        cmake.install()

    def package_info(self):
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.libs = tools.collect_libs(self)
