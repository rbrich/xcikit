#include <xci/graphics/Window.h>
#include <xci/graphics/Renderer.h>
#include <xci/graphics/vulkan/VulkanError.h>
#include <xci/core/ArgParser.h>
#include <xci/core/log.h>

using namespace xci::graphics;
using namespace xci::core::argparser;
using namespace xci::core::log;

inline void setup_window(Window& window, const char* title, const char* argv[])
{
    uint32_t device_id = 0;
    ArgParser {
        Option("-D, --device-id ID", "Select graphics device", device_id),
    } (argv);

    window.renderer().set_device_id(device_id);

    try {
        window.create({800, 600}, title);
    } catch (const VulkanError& e) {
        error("VulkanError: {}", e.what());
        exit(EXIT_FAILURE);
    }
}
