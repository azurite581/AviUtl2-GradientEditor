#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)
#ifndef PLUGIN_VERSION
#define PLUGIN_VERSION "0.5.0"
#endif

namespace scale {
// ImGui::GetFrameHeight() を基準とした相対スケール
namespace relative {
constexpr float GRADIENT_HEIGHT          = 1.5f;
constexpr float GRADIENT_MARGIN_Y        = 0.25f;
constexpr float GRADIENT_MARKER_WIDTH    = 0.5f;
constexpr float GRADIENT_MARKER_HEIGHT   = 0.5f;
constexpr float GRADIENT_MIDPOINT_WIDTH  = 0.5f;
constexpr float GRADIENT_MIDPOINT_HEIGHT = 0.5f;
constexpr float ITEM_NAME_BUTTON_WIDTH   = 5.0f;
constexpr float EFFECT_INDEX_SPIN_WIDTH  = 4.0f;
}  // namespace relative

namespace absolute {
constexpr float FRAME_ROUNDING         = 2.0f;
constexpr float GRAB_MIN_SIZE          = 2.0f;
constexpr float FRAME_BORDER_SIZE      = 1.0f;
constexpr float TAB_ROUNDING           = 0.0f;
constexpr float DOCKING_SEPARATOR_SIZE = 1.0f;
constexpr float ITEM_SPACING_X         = 5.0f;
constexpr float ITEM_SPACING_Y         = 2.0f;
constexpr float ITEM_INNER_SPACING_X   = 1.0f;
constexpr float SCROLLBAR_ROUNDING     = 0.0f;
}  // namespace absolute
}  // namespace scale

constexpr float PRESET_WINDOW_RATIO     = 0.35f;
constexpr float DEFAULT_FONT_SIZE       = 13.0f;
constexpr float ICON_FONT_GLYPHOFFSET_Y = 1.5f;

constexpr const wchar_t* PLUGIN_NAME        = L"Gradient Editor";
constexpr const wchar_t* PLUGIN_INFO        = "Gradient Editor v" WIDEN(PLUGIN_VERSION) " azurite";
constexpr const wchar_t* PLUGIN_FILE_NAME   = L"GradientEditor";
constexpr const wchar_t* WINDOW_NAME        = PLUGIN_NAME;
constexpr const wchar_t* CONFIG_FOLDER_NAME = L"GradientEditorPreset";
constexpr const wchar_t* PRESET_FILE_NAME   = L"gradient_editor_preset.json";
constexpr const wchar_t* HISTORY_FILE_NAME  = L"gradient_editor_history.json";

#if defined(MARKER_COUNT)
inline constexpr uint32_t MAX_MARKER_COUNT = MARKER_COUNT;
#else
inline constexpr uint32_t MAX_MARKER_COUNT = 30;
#endif

inline constexpr const wchar_t* EFFECT_GROUP_NAME = L"@GradientEditor";
inline constexpr const wchar_t* EFFECT_NAMES[]    = {
    L"MultiGradient",
    L"GradientMap"};

constexpr const char* COLOR_SPACE_NAMES[] = {"sRGB", "Linear sRGB", "HSV", "HSL", "L*a*b", "LCh", "Oklab", "Oklch"};
inline const char* INTERP_DIR_NAMES[]     = {reinterpret_cast<const char*>(u8"短経路"), reinterpret_cast<const char*>(u8"長経路")};

#endif  // CONSTANTS_H
