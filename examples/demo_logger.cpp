// demo_logger.cpp created on 2018-03-02, part of XCI toolkit

#include <xci/log/Logger.h>

#include <ostream>

using namespace xci::log;
using std::ostream;

class ArbitraryObject {};
ostream& operator<<(ostream& stream, const ArbitraryObject&) {
    return stream << "I am arbitrary!";
}


int main()
{
    ArbitraryObject obj;

    log_debug("{} {}!", "Hello", "World");
    log_info("float: {} int: {}!", 1.23f, 42);
    log_warning("arbitrary object: {}", obj);
    log_error("beware");
}
