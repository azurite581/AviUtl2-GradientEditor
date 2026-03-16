#ifndef CONFIG2_WRAPPER_H
#define CONFIG2_WRAPPER_H

#include <string>
#include <string_view>

#include "config2_wrapper_interface.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "aviutl2_sdk/config2.h"

class ConfigWrapper : public ConfigWrapperInterface {
public:
    ConfigWrapper(CONFIG_HANDLE* config_handle) : m_config_handle{config_handle} {}

    ConfigWrapper(ConfigWrapper const&)            = delete;
    ConfigWrapper& operator=(ConfigWrapper const&) = delete;
    ConfigWrapper(ConfigWrapper&&)                 = delete;
    ConfigWrapper& operator=(ConfigWrapper&&)      = delete;

private:
    std::string translate(std::wstring_view s) const override
    {
        return toMultiByte(m_config_handle->translate(m_config_handle, s.data()));
    }

    static std::string toMultiByte(std::wstring_view str, uint32_t code_page = CP_UTF8)
    {
        if (str.empty()) return {};

        DWORD flags = 0;
        if (code_page == CP_UTF8) flags = WC_ERR_INVALID_CHARS;

        int32_t size_needed = ::WideCharToMultiByte(code_page, flags, str.data(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
        if (size_needed <= 0) return {};

        std::string result(size_needed, '\0');
        ::WideCharToMultiByte(code_page, flags, str.data(), static_cast<int>(str.size()), result.data(), size_needed, nullptr, nullptr);

        return result;
    };

    CONFIG_HANDLE* m_config_handle = nullptr;
};

#endif
