#ifndef CONFIG2_WRAPPER_INTERFACE_H
#define CONFIG2_WRAPPER_INTERFACE_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string>
#include <string_view>

#include "aviutl2_sdk/config2.h"

class ConfigWrapperInterface {
public:
    virtual ~ConfigWrapperInterface() = default;

    std::string tr(std::wstring_view s)
    {
        return translate(s);
    }

private:
    virtual std::string translate(std::wstring_view s) const = 0;
};

ConfigWrapperInterface* get_config_wrapper_interface(CONFIG_HANDLE* config_handle);

extern ConfigWrapperInterface* config2_wrapper_instance;

#endif
