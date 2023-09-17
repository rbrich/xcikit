include(CheckCXXSourceCompiles)

check_cxx_source_compiles("#include <cstring>
void is_char_p(char *) {}
int main() { char buf[100]; is_char_p(strerror_r(0, buf, 100)); }
" HAVE_GNU_STRERROR_R)

if (NOT HAVE_GNU_STRERROR_R)
    check_cxx_source_compiles("#include <cstring>
    void is_int(int) {}
    int main() { char buf[100]; is_int(strerror_r(0, buf, 100)); }
    " HAVE_XSI_STRERROR_R)
endif()

check_cxx_source_compiles("using float128 = __float128; int main() {}" HAVE_GNU_EXT_FLOAT128)

check_cxx_source_compiles("
#include <string>
#ifdef __GLIBCXX__
int main() {}
#endif
" HAVE_LIBSTDCXX)
