from conans import ConanFile, CMake, tools
import tempfile
import textwrap
import io
import os


class XcikitConan(ConanFile):
    name = "xcikit"
    version = "0.1"
    license = "Apache-2.0"
    author = "Radek Brich"
    url = "https://github.com/rbrich/xcikit"
    description = "Collection of C++ libraries for drawing 2D graphics, rendering text and more."
    topics = ("text-rendering", "ui", "scripting-language", "vulkan", "glsl", "freetype")
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        # Optional components:
        "data": [True, False],
        "script": [True, False],
        "graphics": [True, False],
        "text": [True, False],
        "widgets": [True, False],
        # Also build and install:
        "benchmarks": [True, False],
        "tests": [True, False],
    }
    default_options = {
        "shared": False,
        "data": True,
        "script": True,
        "graphics": True,
        "text": True,
        "widgets": True,
        "benchmarks": True,
        "tests": True,
    }
    requires = (
        'fmt/7.1.3',
    )
    build_requires = (
        'incbin/20180413@rbrich/stable',
        'pfr/1.0.4',
        'magic_enum/0.7.2',
    )
    build_requires_or_preinstalled = (
        # <CMake name>, <min ver>,  <Conan reference>       <option>
        ('range-v3',    '0.10.0',   'range-v3/0.11.0',      None),
        ('Catch2',      '',         'catch2/2.13.4',        'tests'),
        ('benchmark',   '',         'benchmark/1.5.2',      'benchmarks'),
        ('pegtl',       '3.1.0',    'taocpp-pegtl/3.1.0',   None),
        ('glfw3',       '3.2.1',    'glfw/3.3.2',           'graphics'),
    )
    generators = ("cmake_paths", "cmake_find_package")
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

    _cmake = None

    @staticmethod
    def _on_off(flag):
        return 'ON' if flag else 'OFF'

    def _check_option(self, opt):
        """ Check <option> field from `build_requires_or_preinstalled`"""
        return opt is None or self.options.get_safe(opt)

    def _check_preinstalled(self):
        self.output.info(f'Checking for preinstalled dependencies...')
        items = ';'.join(f"{name}/{ver}" for name, ver, _, opt
                         in self.build_requires_or_preinstalled
                         if self._check_option(opt))
        with tempfile.TemporaryDirectory() as tmp_dir:
            with tools.chdir(tmp_dir):
                with open("CMakeLists.txt", 'w') as f:
                    f.write(textwrap.dedent("""
                        cmake_minimum_required(VERSION 3.13)
                        project(SystemPackageFinder CXX)
                        list(APPEND CMAKE_MODULE_PATH """ + os.path.dirname(__file__) + """/cmake)
                        foreach (ITEM IN LISTS DEPS)
                            string(REPLACE "/" ";" ITEM ${ITEM})
                            list(GET ITEM 0 NAME)
                            list(GET ITEM 1 VERSION)
                            find_package(${NAME} ${VERSION})
                            if (${NAME}_FOUND)
                                message("FOUND ${NAME} ${${NAME}_VERSION}")
                            endif()
                        endforeach()
                    """))
                out = io.StringIO()
                if self.run(f"cmake . -G Ninja -DDEPS='{items}'",
                            output=out, ignore_errors=True) != 0:
                    self.output.error(f'Failed:\n{out.getvalue()}')
                    return
                for line in out.getvalue().splitlines():
                    if line.startswith('FOUND '):
                        _, name, version = line.split(' ')
                        self.output.success(f"Found: {name} {version}")
                        yield name

    def build_requirements(self):
        preinstalled = list(self._check_preinstalled())
        for name, _, ref, opt in self.build_requires_or_preinstalled:
            if self._check_option(opt) and name not in preinstalled:
                self.build_requires(ref)

    def requirements(self):
        if self.settings.os == "Windows":
            self.requires("zlib/1.2.11")

    def _configure_cmake(self):
        if not self._cmake:
            self._cmake = CMake(self)
            self._cmake.definitions["CMAKE_INSTALL_PREFIX"] = self.package_folder
            self._cmake.definitions["XCI_SHARE_DIR"] = self.package_folder + "/share/xcikit"
            self._cmake.definitions["XCI_DATA"] = self._on_off(self.options.data)
            self._cmake.definitions["XCI_SCRIPT"] = self._on_off(self.options.script)
            self._cmake.definitions["XCI_GRAPHICS"] = self._on_off(self.options.graphics)
            self._cmake.definitions["XCI_TEXT"] = self._on_off(self.options.text)
            self._cmake.definitions["XCI_WIDGETS"] = self._on_off(self.options.widgets)
            self._cmake.definitions["XCI_BUILD_TESTS"] = self._on_off(self.options.tests)
            self._cmake.definitions["XCI_BUILD_BENCHMARKS"] = self._on_off(self.options.benchmarks)
            self._cmake.configure()
        return self._cmake

    def build(self):
        self.run("./bootstrap.sh --no-conan-remotes")
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def test(self):
        cmake = self._configure_cmake()
        cmake.test()

    def package_info(self):
        self.cpp_info.libs = ["xci-core", "xci-graphics-opengl", "xci-text", "xci-widgets"]
