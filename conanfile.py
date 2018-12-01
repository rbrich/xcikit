from conans import ConanFile, CMake


class XcikitConan(ConanFile):
    name = "xcikit"
    version = "0.1"
    license = "Apache-2.0"
    author = "Radek Brich"
    url = "https://github.com/rbrich/xcikit"
    description = "Toolkit for rendering text and simple UI with OpenGL."
    topics = ("OpenGL", "text-rendering", "UI")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    build_requires = ('catch2/2.2.2@bincrafters/stable',
                      'pegtl/2.7.1@taocpp/stable',
                      'incbin/20180413@rbrich/stable',)
    requires = ('abseil/20180600@bincrafters/stable',
                'glad/0.1.24@bincrafters/stable',)
    default_options = {
        "shared": False,
        # Glad commandline:
        # --profile="core" --api="gl=3.3" --generator="c" --spec="gl" \
        # --local-files --omit-khrplatform --extensions="GL_KHR_debug"
        "glad:spec": "gl",
        "glad:api_type": "gl",
        "glad:api_version": "3.3",
        "glad:profile": "core",
        "glad:extensions": "GL_KHR_debug",
    }
    generators = "cmake_paths"
    scm = {
        "type": "git",
        "url": "auto",
        "revision": "auto"
    }

    def build(self):
        self.run("./bootstrap.sh --no-conan-remotes")
        cmake = CMake(self)
        cmake.definitions["CMAKE_INSTALL_PREFIX"] = self.package_folder
        cmake.definitions["XCI_SHARE_DIR"] = self.package_folder + "/share/xcikit"
        cmake.configure()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["xci-core", "xci-graphics-opengl", "xci-text", "xci-widgets"]
