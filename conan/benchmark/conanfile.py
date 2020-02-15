# modified conanfile for Google Benchmark
# original: https://github.com/google/benchmark/blob/master/conanfile.py
# the customization is needed because the original package misses
# the CMake configs (lib/cmake/benchmark)

from conans import ConanFile, CMake, tools


class GoogleBenchmarkConan(ConanFile):
    name = "benchmark"
    version = "1.5.0"
    description = "A microbenchmark support library"
    homepage = "https://github.com/google/benchmark"
    license = "Apache-2.0"
    exports = ["LICENSE"]
    scm = {
        "type": "git",
        "url": homepage,
        "revision": "v%s" % version,
    }

    # package info
    url = "https://github.com/rbrich/xcikit/tree/master/conan/benchmark"
    author = "Radek Brich"

    settings = "os", "arch", "compiler", "build_type"
    generators = "cmake"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "enable_lto": [True, False],
        "enable_exceptions": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "enable_lto": False,
        "enable_exceptions": True,
    }

    def config_options(self):
        if self.settings.os == 'Windows':
            del self.options.fPIC

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.definitions["BENCHMARK_ENABLE_TESTING"] = False
        cmake.definitions["BENCHMARK_ENABLE_GTEST_TESTS"] = False
        cmake.definitions["BENCHMARK_ENABLE_LTO"] = self.options.enable_lto
        cmake.definitions["BENCHMARK_ENABLE_EXCEPTIONS"] = self.options.enable_exceptions
        cmake.definitions["BENCHMARK_USE_LIBCXX"] = \
            self.settings.os != "Windows" and str(self.settings.compiler.libcxx) == "libc++"
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()
        self.copy("LICENSE", dst="licenses")

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        if self.settings.os == "Linux":
            self.cpp_info.libs.extend(["pthread", "rt"])
        elif self.settings.os == "Windows":
            self.cpp_info.libs.append("shlwapi")
        elif self.settings.os == "SunOS":
            self.cpp_info.libs.append("kstat")
