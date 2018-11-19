#include <iostream>
#include <incbin.h>

INCBIN(Test, "conaninfo.txt");

int main() {
    std::cout << std::string((const char*)gTestData, (size_t)gTestSize) << std::endl;
}
