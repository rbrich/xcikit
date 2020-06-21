from conans import ConanFile, CMake, tools
import tempfile
import textwrap
import io


class XcikitConan(ConanFile):
    name = "xcikit"
    version = "0.1"
    license = "Apache-2.0"
    author = "Radek Brich"
    url = "https://github.com/rbrich/xcikit"
    description = "Collection of C++ libraries for drawing 2D graphics, rendering text and more."
    topics = ("text-rendering", "ui", "scripting-language", "vulkan", "glsl", "freetype")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "benchmarks": [True, False]}
    default_options = {"shared": False, "benchmarks": True}
    build_requires = ('Catch2/2.12.2@rbrich/stable',
                      'pegtl/2.8.0@taocpp/stable',
                      'range-v3/0.10.0@ericniebler/stable',
                      'incbin/20180413@rbrich/stable',
                      'replxx/20200217@rbrich/stable',
                      'magic_get/1.0.0@rbrich/stable',
                      'magic_enum/0.6.6')
    generators = "cmake_paths"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

    def _is_preinstalled(self, name, version=''):
        with tempfile.TemporaryDirectory() as tmp_dir:
            with tools.chdir(tmp_dir):
                with open("CMakeLists.txt", 'w') as f:
                    f.write(textwrap.dedent(f"""
                        cmake_minimum_required(VERSION 3.9)
                        project(SystemPackageFinder CXX)
                        find_package({name} {version} REQUIRED)
                    """))
                out = io.StringIO()
                if self.run("cmake . --log-level=NOTICE", output=out, ignore_errors=True) == 0:
                    self.output.success(f"Found preinstalled dependency: {name}")
                    # `out` is thrown away, it's generally just noise
                    return True
        return False

    def build_requirements(self):
        if self.options.benchmarks and not self._is_preinstalled("benchmark"):
            self.build_requires('benchmark/1.5.0@rbrich/stable')

    def requirements(self):
        if self.settings.os == "Windows":
            self.requires("zlib/1.2.11")

    def build(self):
        self.run("./bootstrap.sh --no-conan-remotes")
        cmake = CMake(self)
        cmake.definitions["CMAKE_INSTALL_PREFIX"] = self.package_folder
        cmake.definitions["XCI_SHARE_DIR"] = self.package_folder + "/share/xcikit"
        cmake.definitions["XCI_BUILD_BENCHMARKS"] = self.options.benchmarks
        cmake.configure()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["xci-core", "xci-graphics-opengl", "xci-text", "xci-widgets"]
