from conans import ConanFile, CMake


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
    build_requires = ('Catch2/2.12.2@rbrich/stable',
                      'pegtl/2.8.0@taocpp/stable',
                      'range-v3/0.10.0@ericniebler/stable',
                      'incbin/20180413@rbrich/stable',
                      'replxx/20200217@rbrich/stable',
                      'benchmark/1.5.0@rbrich/stable',)
    generators = "cmake_paths"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

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
