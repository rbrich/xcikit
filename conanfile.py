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
        'fmt/7.0.1',
    )
    build_requires_or_preinstalled = (
        # <CMake name>, <min ver>,  <Conan reference>
        ('range-v3',    '0.10.0',   'range-v3/0.10.0@ericniebler/stable'),
        ('Catch2',      '',         'catch2/2.12.2'),
        ('benchmark',   '',         'benchmark/1.5.0'),
        ('pegtl',       '2.8.1',    'pegtl/2.8.1@taocpp/stable'),
        ('glfw3',       '3.3.2',    'glfw/3.3.2@rbrich/stable'),
    )
    generators = ("cmake_paths", "cmake_find_package")
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

    def _check_preinstalled(self):
        self.output.info(f'Checking for preinstalled dependencies...')
        items = ';'.join(f"{name}/{ver}" for name, ver, _ in self.build_requires_or_preinstalled)
        with tempfile.TemporaryDirectory() as tmp_dir:
            with tools.chdir(tmp_dir):
                with open("CMakeLists.txt", 'w') as f:
                    f.write(textwrap.dedent("""
                        cmake_minimum_required(VERSION 3.9)
                        project(SystemPackageFinder CXX)
                        foreach (ITEM IN LISTS DEPS)
                            string(REPLACE "/" ";" ITEM ${ITEM})
                            list(GET ITEM 0 NAME)
                            list(GET ITEM 1 VERSION)
                            find_package(${NAME} ${VERSION})
                            if (${NAME}_FOUND)
                                message(NOTICE "FOUND ${NAME} ${${NAME}_VERSION}")
                            endif()
                        endforeach()
                    """))
                out = io.StringIO()
                if self.run(f"cmake . -G Ninja --log-level=NOTICE -DDEPS='{items}'", output=out, ignore_errors=True) != 0:
                    self.output.error(f'Failed:\n{out.getvalue()}')
                    return
                for line in out.getvalue().splitlines():
                    if line.startswith('FOUND '):
                        _, name, version = line.split(' ')
                        self.output.success(f"Found: {name} {version}")
                        yield name

    def build_requirements(self):
        preinstalled = list(self._check_preinstalled())
        for name, _, ref in self.build_requires_or_preinstalled:
            if name not in preinstalled:
                self.build_requires(ref)

    def requirements(self):
        if self.settings.os == "Windows":
            self.requires("zlib/1.2.11")

    def build(self):
        self.run("./bootstrap.sh --no-conan-remotes")
        cmake = CMake(self)
        cmake.definitions["CMAKE_INSTALL_PREFIX"] = self.package_folder
        cmake.definitions["XCI_SHARE_DIR"] = self.package_folder + "/share/xcikit"
        cmake.configure()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["xci-core", "xci-graphics-opengl", "xci-text", "xci-widgets"]
