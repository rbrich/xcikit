#include <xci/graphics/Window.h>
#include <xci/graphics/Renderer.h>
#include <xci/graphics/vulkan/VulkanError.h>
#include <xci/core/ArgParser.h>
#include <xci/core/log.h>

using namespace xci::graphics;
using namespace xci::core;


inline void setup_window(Window& window, const char* title, const char* argv[])
{
    uint32_t device_id = ~0u;

    using namespace xci::core::argparser;
    ArgParser {
        Option("-D, --device-id ID", "Select graphics device", device_id),
    } (argv);

    if (device_id != ~0u)
        window.renderer().set_device_id(device_id);

    if (!window.create({800, 600}, title))
        exit(EXIT_FAILURE);
}
