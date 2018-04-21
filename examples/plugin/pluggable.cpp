#include <string>
#include <xci/util/log.h>

using namespace xci::util::log;

static void lib_load() __attribute__((constructor));
static void lib_load() {
    log_info("pluggable: load");
}

static void lib_unload() __attribute__((destructor));
static void lib_unload() {
    log_info("pluggable: unload");
}

extern "C"
const char* sample_text() {
    return "Hello!";
}
