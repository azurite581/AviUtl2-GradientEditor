#ifndef CONFIG2_UTILS_H
#define CONFIG2_UTILS_H

#include <string>

#include "config2_utils.h"
#include "core/app_state.h"
#include "utils/common/str_conv.h"

namespace aul2 {
inline std::string tr(const wchar_t* key)
{
    if (!gradient_editor::g_app_state.config_handle) {
        return {};
    }
    return str_conv::wideCharToMultiByte(gradient_editor::g_app_state.config_handle->translate(gradient_editor::g_app_state.config_handle, key));
}

inline uint32_t getColor(const std::string& name)
{
    return gradient_editor::g_app_state.config_handle->get_color_code(gradient_editor::g_app_state.config_handle, name.c_str());
}
}  // namespace aul2

#endif  // !CONFIG2_UTILS_H
