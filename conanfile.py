from conans import ConanFile, tools
import conans.model.build_info
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake
import os


class XcikitConan(ConanFile):
    name = "xcikit"
    version = tools.load("VERSION").strip()
    license = "Apache-2.0"
    author = "Radek Brich"
    url = "https://github.com/rbrich/xcikit"
    description = "Collection of C++ libraries for drawing 2D graphics, rendering text and more."
    topics = ("text-rendering", "ui", "scripting-language", "vulkan", "glsl", "freetype")
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        ### Optional components:
        "data": [True, False],
        "script": [True, False],
        "graphics": [True, False],
        "text": [True, False],
        "widgets": [True, False],
        ### Also build and install:
        "tools": [True, False],
        "examples": [True, False],
        "tests": [True, False],
        "benchmarks": [True, False],
        ### System dependencies (instead of Conan):
        "system_fmt": [True, False],
        "system_zlib": [True, False],
        "system_glfw": [True, False],
        "system_vulkan": [True, False],
        "system_freetype": [True, False],
        "system_harfbuzz": [True, False],
        # Hyperscan is never installed via Conan, because it would bring ton of deps
        # and it's only needed for optional ff tool.
        # This option enables using system-installed Hyperscan.
        "with_hyperscan": [True, False],
        "system_boost": [True, False],
        "system_range_v3": [True, False],
        "system_catch2": [True, False],
        "system_benchmark": [True, False],
        "system_pegtl": [True, False],
        "system_magic_enum": [True, False],
    }
    default_options = {
        "shared": False,
        "data": True,
        "script": True,
        "graphics": True,
        "text": True,
        "widgets": True,
        "tools": True,
        "examples": True,
        "tests": True,
        "benchmarks": True,
        "system_fmt": False,
        "system_zlib": False,
        "system_glfw": False,
        "system_vulkan": False,
        "system_freetype": False,
        "system_harfbuzz": False,
        "with_hyperscan": False,
        "system_boost": False,
        "system_range_v3": False,
        "system_catch2": False,
        "system_benchmark": False,
        "system_pegtl": False,
        "system_magic_enum": False,

        # Disable unnecessary transient deps by default.
        "freetype:with_png": True,
        "freetype:with_zlib": False,
        "freetype:with_bzip2": False,
        "freetype:with_brotli": False,
        "harfbuzz:with_glib": False,
        "vulkan-loader:with_wsi_xcb": False,
        "vulkan-loader:with_wsi_xlib": False,
        "vulkan-loader:with_wsi_wayland": False,
        "vulkan-loader:with_wsi_directfb": False,
    }

    exports = ("VERSION", "requirements.csv")
    exports_sources = ("CMakeLists.txt", "config.h.in", "xcikit-config.cmake.in",
                       "cmake/**", "src/**", "examples/**", "tests/**", "benchmarks/**", "tools/**",
                       "share/**", "third_party/**",
                       "!build/**", "!cmake-build-*/**")
    revision_mode = "scm"

    _cmake = None

    def _check_option(self, prereq, option=None):
        """ Check <prereq. option> and <system option> fields from `requirements.csv`"""
        return (
            (not prereq or self.options.get_safe(prereq)) and  # check prerequisite
            (not option or not self.options.get_safe(option))  # not using system lib
        )

    def config_options(self):
        if self.settings.os != "Linux":
            del self.options["vulkan-loader"].with_wsi_xcb
            del self.options["vulkan-loader"].with_wsi_xlib
            del self.options["vulkan-loader"].with_wsi_wayland
            del self.options["vulkan-loader"].with_wsi_directfb

    def configure(self):
        if not self.options.graphics:
            del self.options.text
            del self.options.widgets
        elif not self.options.text:
            del self.options.widgets
        for _, _, _, _, prereq, option in self._requirements_csv():
            if not self._check_option(prereq):
                self.options.remove(option)

    def _requirements_csv(self):
        import csv
        from pathlib import Path
        script_dir = Path(__file__).parent
        with open(script_dir.joinpath('requirements.csv'), newline='') as csvfile:
            reader = csv.reader(csvfile, delimiter=',', quotechar='"')
            for row in reader:
                if row[0].strip()[0] in '<#':
                    continue  # header or comment
                row = [x.strip() for x in row]
                yield row

    def requirements(self):
        for br, _, _, ref, prereq, system in self._requirements_csv():
            if ref and br == 'run' and self._check_option(prereq, system):
                self.requires(ref)

    def build_requirements(self):
        for br, _, _, ref, prereq, system in self._requirements_csv():
            if ref and br == 'build' and self._check_option(prereq, system):
                self.build_requires(ref)

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")
        if self.package_folder:
            tc.variables["XCI_SHARE_DIR"] = self.package_folder + "/share/xcikit"
        tc.variables["XCI_DATA"] = self.options.data
        tc.variables["XCI_SCRIPT"] = self.options.script
        tc.variables["XCI_GRAPHICS"] = self.options.graphics
        tc.variables["XCI_TEXT"] = self.options.get_safe('text', False)
        tc.variables["XCI_WIDGETS"] = self.options.get_safe('widgets', False)
        tc.variables["XCI_BUILD_TOOLS"] = self.options.tools
        tc.variables["XCI_BUILD_EXAMPLES"] = self.options.examples
        tc.variables["XCI_BUILD_TESTS"] = self.options.tests
        tc.variables["XCI_BUILD_BENCHMARKS"] = self.options.benchmarks
        tc.variables["XCI_WITH_HYPERSCAN"] = self.options.get_safe('with_hyperscan', False)
        tc.generate()

        deps = CMakeDeps(self)
        deps.build_context_activated = ['catch2', 'benchmark']
        deps.generate()

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def test(self):
        cmake = self._configure_cmake()
        cmake.test()

    def _x_layout(self):
        # DISABLED: This is very experimental for now, breaks the whole package when enabled.
        # Support for editable mode
        build_dir = os.environ.get('CONAN_EDITABLE_BUILD_DIR', None)
        if build_dir is not None:
            self.folders.build = build_dir # e.g. "build/macos-x86_64-Release-Ninja"
            self.folders.generators = "generators"

        for name in ('core', 'data', 'script', 'graphics', 'text', 'widgets'):
            if name != 'core' and not self.options.get_safe(name, False):
                continue  # component is disabled
            component = self.cpp.source.components["xci-" + name]
            component.libdirs = [self.folders.build + '/src/xci/' + name]
            component.includedirs = ["src", self.folders.build + '/include']
            component.builddirs = ["cmake"]

    def _add_dep(self, opt: str, component: conans.model.build_info.Component, cmake_dep: str):
        if self.options.get_safe(opt, False):
            component.system_libs = [cmake_dep]
        else:
            component.requires += [cmake_dep]

    def package_info(self):
        for name in ('core', 'data', 'script', 'graphics', 'text', 'widgets'):
            if name != 'core' and not self.options.get_safe(name, False):
                continue  # component is disabled
            component = self.cpp_info.components["xci-" + name]
            component.libs = ["xci-" + name]
            component.libdirs = ["lib"]
            component.includedirs = ["include"]
            component.builddirs = ["lib/cmake/xcikit"]
            if name == 'core':
                self._add_dep('system_fmt', component, "fmt::fmt")
                self._add_dep('system_magic_enum', component, "magic_enum::magic_enum")
            if name == 'data':
                self._add_dep('system_boost', component, "pfr::pfr")
            if name == 'script':
                component.requires += ['xci-core']
            if name == 'graphics':
                component.requires += ["xci-core"]
                self._add_dep('system_glfw', component, "glfw::glfw")
                self._add_dep('system_vulkan', component, "vulkan-loader::vulkan-loader")
            if name == 'text':
                component.requires += ['xci-core', 'xci-graphics']
                self._add_dep('system_freetype', component, "freetype::freetype")
                self._add_dep('system_harfbuzz', component, "harfbuzz::harfbuzz")
            if name == 'widgets':
                component.requires += ['xci-text']
