from conans import ConanFile, CMake, tools


class XcikitConan(ConanFile):
    name = "xcikit"
    version = tools.load("VERSION").strip()
    license = "Apache-2.0"
    author = "Radek Brich"
    url = "https://github.com/rbrich/xcikit"
    description = "Collection of C++ libraries for drawing 2D graphics, rendering text and more."
    topics = ("text-rendering", "ui", "scripting-language", "vulkan", "glsl", "freetype")
    settings = "os", "compiler", "build_type", "arch"
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
        "system_zlib": [True, False],
        "system_glfw": [True, False],
        "system_vulkan": [True, False],
        "system_freetype": [True, False],
        # Hyperscan is never installed via Conan, because it would bring ton of deps
        # and it's only needed for optional ff tool.
        # This option enables using system-installed Hyperscan.
        "with_hyperscan": [True, False],
        "system_boost": [True, False],
        "system_range_v3": [True, False],
        "system_catch2": [True, False],
        "system_benchmark": [True, False],
        "system_pegtl": [True, False],
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
        "system_zlib": False,
        "system_glfw": False,
        "system_vulkan": False,
        "system_freetype": False,
        "with_hyperscan": False,
        "system_boost": False,
        "system_range_v3": False,
        "system_catch2": False,
        "system_benchmark": False,
        "system_pegtl": False,
    }
    requires = (
        'fmt/8.0.1',
    )
    build_requires = (
        'magic_enum/0.7.2',
    )

    exports = ("VERSION", "requirements.csv")
    exports_sources = ("bootstrap.sh", "CMakeLists.txt", "config.h.in", "xcikitConfig.cmake.in",
                       "cmake/**", "src/**", "examples/**", "tests/**", "benchmarks/**", "tools/**",
                       "share/**", "third_party/**")
    revision_mode = "scm"

    _cmake = None

    @staticmethod
    def _on_off(flag):
        return 'ON' if flag else 'OFF'

    def _check_option(self, prereq, option=None):
        """ Check <prereq. option> and <system option> fields from `requirements.csv`"""
        return (
            (not prereq or self.options.get_safe(prereq)) and  # check prerequisite
            (not option or not self.options.get_safe(option))  # not using system lib
        )

    def config_options(self):
        for _, _, _, _, prereq, system in self._requirements_csv():
            if not self._check_option(prereq):
                self.options.remove(system)

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
            self._cmake.definitions["XCI_BUILD_TOOLS"] = self._on_off(self.options.tools)
            self._cmake.definitions["XCI_BUILD_EXAMPLES"] = self._on_off(self.options.examples)
            self._cmake.definitions["XCI_BUILD_TESTS"] = self._on_off(self.options.tests)
            self._cmake.definitions["XCI_BUILD_BENCHMARKS"] = self._on_off(self.options.benchmarks)
            self._cmake.definitions["XCI_WITH_HYPERSCAN"] = self._on_off(self.options.with_hyperscan)
            self._cmake.configure()
        return self._cmake

    def build(self):
        self.run("./bootstrap.sh")
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
