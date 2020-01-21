#include <string>
#include <xci/core/log.h>

using namespace xci::core::log;

#ifdef _WIN32

#include <windows.h>

#define EXPORT __declspec(dllexport)

BOOL WINAPI DllMain(
        _In_ HINSTANCE hinstDLL [[maybe_unused]],
        _In_ DWORD     fdwReason,
        _In_ LPVOID    lpvReserved [[maybe_unused]])
{
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH: log_info("pluggable: load"); break;
        case DLL_PROCESS_DETACH: log_info("pluggable: unload"); break;
        default: break;
    }
    return TRUE;
}

#else

#define EXPORT

static void lib_load() __attribute__((constructor));
static void lib_load() {
    log_info("pluggable: load");
}

static void lib_unload() __attribute__((destructor));
static void lib_unload() {
    log_info("pluggable: unload");
}

#endif

extern "C"
EXPORT const char* sample_text() {
    return "Hello!";
}
