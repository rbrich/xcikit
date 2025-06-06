import os

from conan import ConanFile
from conan.tools.files import load
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout
from conan.tools.files import copy
from conan.tools.microsoft import is_msvc_static_runtime, is_msvc
from pathlib import Path

required_conan_version = ">=2.0.0"


class XcikitConan(ConanFile):
    name = "xcikit"
    license = "Apache-2.0"
    author = "Radek Brich"
    url = "https://github.com/rbrich/xcikit"
    homepage = url
    description = "Collection of C++ libraries for drawing 2D graphics, rendering text and more."
    topics = ("text-rendering", "ui", "scripting-language", "vulkan", "glsl", "freetype")
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],

        # Optional components:
        "vfs": [True, False],
        "data": [True, False],
        "script": [True, False],
        "graphics": [True, False],
        "text": [True, False],
        "widgets": [True, False],

        # Also build and install:
        "tools": [True, False],
        "examples": [True, False],
        "tests": [True, False],
        "benchmarks": [True, False],

        # Individual tools ("tools" must be enabled too)
        "dar_tool": [True, False],
        "dati_tool": [True, False],
        "ff_tool": [True, False],
        "fire_tool": [True, False],
        "shed_tool": [True, False],
        "tc_tool": [True, False],

        # System dependencies (instead of Conan):
        "system_fmt": [True, False],
        "system_zlib": [True, False],
        "system_libzip": [True, False],
        "system_sdl": [True, False],
        "system_vulkan": [True, False],
        "system_spirv-cross": [True, False],
        "system_freetype": [True, False],
        "system_harfbuzz": [True, False],
        "system_pfr": [True, False],
        "system_range_v3": [True, False],
        "system_catch2": [True, False],
        "system_benchmark": [True, False],
        "system_pegtl": [True, False],
        "system_magic_enum": [True, False],
        # Hyperscan is never installed via Conan, because it would bring ton of deps,
        # and it's only needed for optional ff tool.
        # This option enables using system-installed Hyperscan.
        "with_hyperscan": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,

        # Optional components:
        "vfs": True,
        "data": True,
        "script": True,
        "graphics": True,
        "text": True,
        "widgets": True,

        # Also build and package:
        "tools": True,
        "examples": True,
        "tests": True,
        "benchmarks": True,

        # Individual tools
        "dar_tool": True,
        "dati_tool": True,
        "ff_tool": True,
        "fire_tool": True,
        "shed_tool": True,
        "tc_tool": True,

        # System dependencies (instead of Conan):
        "system_fmt": False,
        "system_zlib": False,
        "system_libzip": False,
        "system_sdl": False,
        "system_vulkan": False,
        "system_spirv-cross": False,
        "system_freetype": False,
        "system_harfbuzz": False,
        "system_pfr": False,
        "system_range_v3": False,
        "system_catch2": False,
        "system_benchmark": False,
        "system_pegtl": False,
        "system_magic_enum": False,
        "with_hyperscan": False,

        # Disable unnecessary transient deps by default.
        "libzip/*:with_lzma": False,
        "libzip/*:with_zstd": False,
        "libzip/*:crypto": False,
        "libzip/*:tools": False,
        "freetype/*:with_brotli": False,
        "harfbuzz/*:with_glib": False,
        "vulkan-loader/*:with_wsi_xcb": False,
        "vulkan-loader/*:with_wsi_xlib": False,
        "vulkan-loader/*:with_wsi_wayland": False,
        "vulkan-loader/*:with_wsi_directfb": False,
    }

    exports_sources = ("VERSION", "CMakeLists.txt", "config.h.in", "xcikit-config.cmake.in",
                       "cmake/**", "src/**", "examples/**", "tests/**", "benchmarks/**", "tools/**",
                       "share/**", "third_party/**",
                       "!build/**", "!cmake-build-*/**")

    def set_version(self):
        self.version = load(self, Path(self.recipe_folder) / "VERSION").strip()

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def layout(self):
        # Explicit BUILD_DIR passed from build.sh
        build_dir = os.environ.get("BUILD_DIR", None)
        if build_dir:
            self.folders.source = "."
            self.folders.build = build_dir
            self.folders.generators = os.path.join(self.folders.build, "generators")
            self.cpp.source.includedirs = ["include"]
            self.cpp.build.bindirs = ["."]
            self.cpp.build.libdirs = ["."]
        else:
            cmake_layout(self)

    def _requirements(self):
        for name, info in self.conan_data["requirements"].items():
            info['name'] = name
            info['option'] = f"system_{name}" if 'conan' in info else f"with_{name}"
            info.setdefault('prereq', [])
            info.setdefault('public', False)
            info.setdefault('conan_traits', [])
            yield info

    def validate(self):
        check_min_cppstd(self, "20")

    @staticmethod
    def _check_prereq(prereq, options):
        if not prereq:
            return True
        for p in prereq:
            if options[p]:
                return True
        return False

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        # evaluated options for _check_prereq
        options = {k: v[0] == 'T' for k, v in self.options.items()}
        # Dependent options
        if options['widgets']:
            del self.options.text
            options['text'] = True
        if options['text']:
            del self.options.graphics
            options['graphics'] = True
        if options['script']:
            del self.options.data
            options['data'] = True
        if options['script'] or options['graphics']:
            del self.options.vfs
            options['vfs'] = True
        if not options['tools']:
            del self.options.dar_tool
            del self.options.dati_tool
            del self.options.ff_tool
            del self.options.fire_tool
            del self.options.shed_tool
            del self.options.tc_tool
            options['dar_tool'] = False
            options['dati_tool'] = False
            options['ff_tool'] = False
            options['fire_tool'] = False
            options['shed_tool'] = False
            options['tc_tool'] = False
        else:
            if not options['widgets']:
                del self.options.shed_tool
                options['shed_tool'] = False
            if not options['script']:
                del self.options.fire_tool
                options['fire_tool'] = False
            if not options['data']:
                del self.options.dati_tool
                options['dati_tool'] = False
            if not options['vfs']:
                del self.options.dar_tool
                options['dar_tool'] = False
            if not options['with_hyperscan']:
                del self.options.ff_tool
                options['ff_tool'] = False

        # Remove system_ options for disabled components
        for info in self._requirements():
            if not self._check_prereq(info['prereq'], options):
                delattr(self.options, info['option'])

        # Remove dependent / implicit options
        if self.settings.os == "Emscripten" and 'system_zlib' in self.options:
            # These are imported from Emscripten Ports
            del self.options.system_zlib

    def requirements(self):
        for info in self._requirements():
            # Install requirement via Conan if `system_<lib>` option exists and is set to False
            opt = self.options.get_safe('system_' + info['name'])
            if opt is not None and not opt:
                traits = {k : {'False':False, 'True':True}[v] for k, v in
                          (it.split('=', 1) for it in info['conan_traits'])}
                if 'tests' in info['prereq'] or 'benchmarks' in info['prereq']:
                    self.test_requires(info['conan'], **traits)
                elif info['public']:
                    self.requires(info['conan'], transitive_headers=True, transitive_libs=True, **traits)
                else:
                    self.requires(info['conan'], headers=True, libs=True, **traits)

    def _set_cmake_defs(self, defs):
        if self.package_folder:
            defs["XCI_SHARE_DIR"] = self.package_folder + "/share/xcikit"
        defs["XCI_DATA"] = self.options.get_safe('data', True)
        defs["XCI_SCRIPT"] = self.options.script
        defs["XCI_GRAPHICS"] = self.options.get_safe('graphics', True)
        defs["XCI_TEXT"] = self.options.get_safe('text', True)
        defs["XCI_WIDGETS"] = self.options.widgets
        defs["BUILD_TOOLS"] = self.options.tools
        defs["BUILD_EXAMPLES"] = self.options.examples
        defs["BUILD_TESTS"] = self.options.tests
        defs["BUILD_BENCHMARKS"] = self.options.benchmarks
        defs["BUILD_DAR_TOOL"] = self.options.get_safe('dar_tool', True)
        defs["BUILD_DATI_TOOL"] = self.options.get_safe('dati_tool', True)
        defs["BUILD_FF_TOOL"] = self.options.get_safe('ff_tool', True)
        defs["BUILD_FIRE_TOOL"] = self.options.get_safe('fire_tool', True)
        defs["BUILD_SHED_TOOL"] = self.options.get_safe('shed_tool', True)
        defs["BUILD_TC_TOOL"] = self.options.get_safe('tc_tool', True)
        defs["XCI_WITH_HYPERSCAN"] = self.options.get_safe('with_hyperscan', False)

    def generate(self):
        tc = CMakeToolchain(self, generator="Ninja")
        self._set_cmake_defs(tc.variables)
        if is_msvc(self):
            tc.variables["USE_MSVC_RUNTIME_LIBRARY_DLL"] = not is_msvc_static_runtime(self)
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, pattern="LICENSE", dst=Path(self.package_folder, "licenses"),
             src=self.source_folder)
        cmake = CMake(self)
        cmake.install()

    def test(self):
        cmake = CMake(self)
        cmake.test()

    def _add_dep(self, opt: str, component,
                 cmake_dep: str, conan_dep=None):
        opt_val = self.options.get_safe(opt)
        if opt_val is None:  # system option deleted
            return
        if opt_val:
            component.system_libs = [cmake_dep]
        else:
            component.requires += [conan_dep if conan_dep is not None else cmake_dep]

    def package_info(self):
        for name in ('core', 'vfs', 'data', 'script', 'graphics', 'text', 'widgets'):
            if not self.options.get_safe(name, True):
                continue  # component is disabled
            component = self.cpp_info.components["xci-" + name]
            component.libs = ["xci-" + name]
            component.libdirs = ["lib"]
            component.includedirs = ["include"]
            component.builddirs = ["lib/cmake/xcikit"]
            if name == 'core':
                self._add_dep('system_fmt', component, "fmt::fmt")
                self._add_dep('system_magic_enum', component, "magic_enum::magic_enum")
                self._add_dep('system_pegtl', component, "taocpp::pegtl", "taocpp-pegtl::taocpp-pegtl")
            if name == 'vfs':
                self._add_dep('system_libzip', component, "libzip::zip", "libzip::libzip")
            if name == 'data':
                self._add_dep('system_zlib', component, "zlib::zlib")
                self._add_dep('system_pfr', component, "pfr::pfr")
            if name == 'script':
                component.requires += ['xci-core']
                self._add_dep('system_range_v3', component, "range-v3::range-v3")
            if name == 'graphics':
                component.requires += ["xci-core"]
                self._add_dep('system_sdl', component, "SDL3::SDL3", "sdl::sdl")
                self._add_dep('system_vulkan', component, "vulkan-loader::vulkan-loader")
                self._add_dep('system_range_v3', component, "range-v3::range-v3")
            if name == 'text':
                component.requires += ['xci-core', 'xci-graphics']
                self._add_dep('system_freetype', component, "freetype::freetype")
                self._add_dep('system_harfbuzz', component, "harfbuzz::harfbuzz")
            if name == 'widgets':
                component.requires += ['xci-text']
