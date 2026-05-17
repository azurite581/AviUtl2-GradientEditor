#include "config2_wrapper_interface.h"

#include "config2_wrapper.h"

ConfigWrapperInterface* config2_wrapper_instance = nullptr;

ConfigWrapperInterface* get_config_wrapper_interface(CONFIG_HANDLE* config_handle)
{
    [[maybe_unused]] static bool init = [config_handle]() {
        if (!config2_wrapper_instance) {
            static ConfigWrapper config_wrapper(config_handle);
            config2_wrapper_instance = &config_wrapper;
        }
        return true;
    }();

    return config2_wrapper_instance;
}
