#include <xci/graphics/Window.h>
#include <xci/graphics/Renderer.h>
#include <xci/graphics/vulkan/VulkanError.h>
#include <xci/core/ArgParser.h>
#include <xci/config/Config.h>
#include <xci/core/log.h>

using namespace xci::graphics;
using namespace xci::core;
using namespace xci::config;


inline void setup_window(Window& window, const char* title, const char* argv[])
{
    uint32_t device_id = ~0u;
    fs::path config_file;

    using namespace xci::core::argparser;
    ArgParser {
        Option("-c, --config FILE", "Load config file", config_file),
        Option("-D, --device-id ID", "Select graphics device", device_id),
    } (argv);

    if (!config_file.empty()) {
        Config conf;
        if (!conf.parse_file(config_file))
            exit(EXIT_FAILURE);
        if (auto v = conf["device_id"].to_int(); v != 0)
            window.renderer().set_device_id(v);
        if (auto v = conf["fullscreen_mode"].to_int(); v >= 0 && v < 4)
            window.set_fullscreen_mode(FullscreenMode(v));
    }

    if (device_id != ~0u)
        window.renderer().set_device_id(device_id);

    if (!window.create({800, 600}, title))
        exit(EXIT_FAILURE);
}
