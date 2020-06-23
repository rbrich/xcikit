# Recipe from conan-center, adapted to allow headless build, using OSmesa.

import os
import glob
from conans import ConanFile, CMake, tools


class GlfwConan(ConanFile):
    name = "glfw"
    version = "3.3.2"
    sha256 = '33c6bfc422ca7002befbb39a7a60dc101d32c930ee58be532ffbcd52e9635812'
    description = "GLFW is a free, Open Source, multi-platform library for OpenGL, OpenGL ES and Vulkan" \
                  "application development. It provides a simple, platform-independent API for creating" \
                  "windows, contexts and surfaces, reading input, handling events, etc."
    homepage = "https://github.com/glfw/glfw"
    topics = ("gflw", "vulkan")
    settings = "os", "arch", "build_type", "compiler"
    license = "Zlib"

    # package info
    url = "https://github.com/rbrich/xcikit/tree/master/conan/glfw"
    author = "Radek Brich, based on conan-center"

    options = {"shared": [True, False], "fPIC": [True, False], "OSMesa": [False, True]}
    default_options = {"shared": False, "fPIC": True, "OSMesa": True}
    _source_dir = f"{name}-{version}"
    _cmake = None

    def _configure_cmake(self):
        if not self._cmake:
            self._cmake = CMake(self)
            self._cmake.definitions["GLFW_BUILD_EXAMPLES"] = False
            self._cmake.definitions["GLFW_BUILD_TESTS"] = False
            self._cmake.definitions["GLFW_BUILD_DOCS"] = False
            self._cmake.definitions["GLFW_USE_OSMESA"] = self.options.OSMesa
            if self.settings.compiler == "Visual Studio":
                self._cmake.definitions["USE_MSVC_RUNTIME_LIBRARY_DLL"] = "MD" in self.settings.compiler.runtime
            self._cmake.configure(source_folder=self._source_dir)
        return self._cmake

    def requirements(self):
        if not self.options.OSMesa:
            self.requires("opengl/system")
            if self.settings.os == "Linux":
                self.requires("xorg/system")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        del self.settings.compiler.libcxx
        del self.settings.compiler.cppstd

    def source(self):
        tools.get(url=f'https://github.com/glfw/glfw/archive/{self.version}.zip',
                  sha256=self.sha256)

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
        if self.settings.os == "Macos" and self.options.shared:
            with tools.chdir(os.path.join(self._source_dir, 'src')):
                for filename in glob.glob('*.dylib'):
                    self.run('install_name_tool -id {filename} {filename}'.format(filename=filename))

    def package(self):
        self.copy("LICENSE*", dst="licenses", src=self._source_dir)
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        if self.settings.os == "Linux":
            self.cpp_info.system_libs.extend(["m", "pthread", "dl", "rt"])
        elif self.settings.os == "Macos":
            self.cpp_info.frameworks.extend(["Cocoa", "IOKit", "CoreFoundation"])
