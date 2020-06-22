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
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    build_requires = (
        'incbin/20180413@rbrich/stable',
        'replxx/20200217@rbrich/stable',
        'magic_get/1.0.0@rbrich/stable',
        'magic_enum/0.6.6',
    )
    build_requires_or_preinstalled = (
        # CMake find name, Conan reference
        ('range-v3', 'range-v3/0.10.0@ericniebler/stable'),
        ('Catch2', 'catch2/2.12.2'),
        ('benchmark', 'benchmark/1.5.0'),
        ('pegtl', 'pegtl/2.8.1@taocpp/stable'),
    )
    generators = ("cmake_paths", "cmake_find_package")
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
                if self.run("cmake . -G Ninja --log-level=NOTICE", output=out, ignore_errors=True) == 0:
                    self.output.success(f"Found preinstalled dependency: {name}")
                    # `out` is thrown away, it's generally just noise
                    return True
                self.output.info(f'Will get dependency via Conan: {name}')
                return False

    def build_requirements(self):
        for name, ref in self.build_requires_or_preinstalled:
            if not self._is_preinstalled(name):
                self.build_requires(ref)

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
