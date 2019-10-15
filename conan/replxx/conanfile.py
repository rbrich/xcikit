from conans import ConanFile, CMake, tools


class ReplxxConan(ConanFile):
    name = "replxx"
    description = "A readline and libedit replacement that supports UTF-8, " \
                  "syntax highlighting, hints and Windows and is BSD licensed."
    homepage = "https://github.com/AmokHuginnsson/replxx"
    version = "20190926"
    license = "BSD"
    _source_commit = "c8fbb3af03cd151e935f556237d6617961244095"

    # package info
    url = "https://github.com/rbrich/xcikit/tree/master/conan/replxx"
    author = "Radek Brich"

    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    generators = "cmake"

    def source(self):
        self.run(f"git clone {self.homepage}")
        self.run(f"cd {self.name} && git checkout {self._source_commit}")

    def build(self):
        cmake = CMake(self)
        cmake.configure(source_dir=self.name)
        cmake.build()
        cmake.install()

    def package_info(self):
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.libs = tools.collect_libs(self)
