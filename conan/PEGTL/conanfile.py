# https://bintray.com/conan/conan-transit/PEGTL%3ADEGoodmanWilson

from conans import ConanFile, tools


class PEGTLConan(ConanFile):
    name = "PEGTL"
    version = "2.4.0"
    license = "MIT"
    username = "taocpp"
    url = "https://github.com/%s/PEGTL.git" % username

    def source(self):
        tools.download("https://github.com/%s/PEGTL/archive/%s.zip"
                       % (self.username, self.version),
                       "PEGTL.zip")
        tools.unzip("PEGTL.zip")

    def package(self):
        self.copy("*", "include/tao", "PEGTL-%s/include/tao" % self.version)
