#include <xci/core/log.h>
#include <string>

using namespace xci::core::log;

#ifdef _WIN32

#include <xci/compat/windows.h>

#define EXPORT __declspec(dllexport)

BOOL WINAPI DllMain(
        _In_ HINSTANCE hinstDLL [[maybe_unused]],
        _In_ DWORD     fdwReason,
        _In_ LPVOID    lpvReserved [[maybe_unused]])
{
    switch(fdwReason) {
        case DLL_PROCESS_ATTACH: info("pluggable: load"); break;
        case DLL_PROCESS_DETACH: info("pluggable: unload"); break;
        default: break;
    }
    return TRUE;
}

#else

#define EXPORT

static void lib_load() __attribute__((constructor));
static void lib_load() {
    info("pluggable: load");
}

static void lib_unload() __attribute__((destructor));
static void lib_unload() {
    info("pluggable: unload");
}

#endif

extern "C"
EXPORT const char* sample_text() {
    return "Hello!";
}
