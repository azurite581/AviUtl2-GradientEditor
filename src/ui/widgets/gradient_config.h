#ifndef GRADIENT_CONFIG_H
#define GRADIENT_CONFIG_H

#include <compare>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "color_conv.h"
#include "gradient_data.h"
#include "gradient_marker.h"
#include "json.hpp"
#include "str_conv.h"

// 共通の結果型
template <typename T>
struct ConfigLoadResult {
    T config;
    std::string error; // 空なら成功
    bool is_success() const { return error.empty(); }
};

struct ConfigWriteResult {
    bool is_success;
    std::string error; // 空なら成功
};

struct GradientPreset {
    std::string category{"Uncategorized"};
    std::string name{"default"};
    std::vector<std::string> colors{"0x000000ff", "0xffffffff"};
    std::vector<float> positions{0.0f, 1.0f};
    std::vector<float> midpoints{0.5f};
    std::vector<float> alpha_values{1.0f, 1.0f};
    std::vector<float> alpha_positions{0.0f, 1.0f};
    float blur_width{1.0f};
    int32_t color_space{0};
    int32_t interpolation_path{0};
};

inline void to_json(nlohmann::ordered_json& j, const GradientPreset& preset)
{
    j = nlohmann::ordered_json{
        {"category", preset.category},
        {"name", preset.name},
        {"colors", preset.colors},
        {"positions", preset.positions},
        {"midpoints", preset.midpoints},
        {"alpha_values", preset.alpha_values},
        {"alpha_positions", preset.alpha_positions},
        {"blur_width", preset.blur_width},
        {"color_space", preset.color_space},
        {"interpolation_path", preset.interpolation_path}};
}

inline void from_json(const nlohmann::ordered_json& j, GradientPreset& preset)
{
    preset.category           = j.value("category", preset.category);
    preset.name               = j.value("name", preset.name);
    preset.colors             = j.value("colors", preset.colors);
    preset.positions          = j.value("positions", preset.positions);
    preset.midpoints          = j.value("midpoints", preset.midpoints);
    preset.alpha_values       = j.value("alpha_values", preset.alpha_values);
    preset.alpha_positions    = j.value("alpha_positions", preset.alpha_positions);
    preset.blur_width         = j.value("blur_width", preset.blur_width);
    preset.color_space        = j.value("color_space", preset.color_space);
    preset.interpolation_path = j.value("interpolation_path", preset.interpolation_path);
}

struct GradientHistory {
    std::string name{"default"};
    std::vector<std::string> colors{"0x000000ff", "0xffffffff"};
    std::vector<float> positions{0.0f, 1.0f};
    std::vector<float> midpoints{0.5f};
    std::vector<float> alpha_values{1.0f, 1.0f};
    std::vector<float> alpha_positions{0.0f, 1.0f};
    float blur_width{1.0f};
    int32_t color_space{0};
    int32_t interpolation_path{0};

    auto operator<=>(const GradientHistory&) const = default;
};

inline void to_json(nlohmann::ordered_json& j, const GradientHistory& history)
{
    j = nlohmann::ordered_json{
        {"name", history.name},
        {"colors", history.colors},
        {"positions", history.positions},
        {"midpoints", history.midpoints},
        {"alpha_values", history.alpha_values},
        {"alpha_positions", history.alpha_positions},
        {"blur_width", history.blur_width},
        {"color_space", history.color_space},
        {"interpolation_path", history.interpolation_path}};
}

inline void from_json(const nlohmann::ordered_json& j, GradientHistory& history)
{
    history.name               = j.value("name", history.name);
    history.colors             = j.value("colors", history.colors);
    history.positions          = j.value("positions", history.positions);
    history.midpoints          = j.value("midpoints", history.midpoints);
    history.alpha_values       = j.value("alpha_values", history.alpha_values);
    history.alpha_positions    = j.value("alpha_positions", history.alpha_positions);
    history.blur_width         = j.value("blur_width", history.blur_width);
    history.color_space        = j.value("color_space", history.color_space);
    history.interpolation_path = j.value("interpolation_path", history.interpolation_path);
}

struct PresetConfig {
    int32_t selected_category = 0;
    std::vector<std::string> categories{"Uncategorized"};
    std::vector<GradientPreset> presets{GradientPreset{}};
};

inline void to_json(nlohmann::ordered_json& j, const PresetConfig& cfg)
{
    j = nlohmann::ordered_json{
        {"selected_category", cfg.selected_category},
        {"categories", cfg.categories.empty() ? std::vector<std::string>{"Uncategorized"} : cfg.categories},
        {"presets",    cfg.presets.empty()     ? std::vector<GradientPreset>{GradientPreset{}} : cfg.presets}};
}

inline void from_json(const nlohmann::ordered_json& j, PresetConfig& cfg)
{
    cfg.selected_category = j.value("selected_category", cfg.selected_category);
    cfg.categories = j.value("categories", cfg.categories);
    cfg.presets    = j.value("presets",    cfg.presets);
}

struct HistoryConfig {
    std::vector<GradientHistory> histories{};
};

inline void to_json(nlohmann::ordered_json& j, const HistoryConfig& cfg)
{
    j = nlohmann::ordered_json{{"histories", cfg.histories}};
}

inline void from_json(const nlohmann::ordered_json& j, HistoryConfig& cfg)
{
    cfg.histories = j.value("histories", cfg.histories);
}

class GradientConfigManager {
public:
    inline static const char* DEFAULT_CATEGORY = "Uncategorized";

    // デフォルトのプリセットファイルが存在しない場合に書き出すプリセット
    static constexpr const char* DEFAULT_PRESET_FILE_JSON = R"(
{
    "categories": [
        "Basic"
    ],
    "presets": [
        {
            "category": "Basic",
            "name": "Basic01",
            "colors": [
                "0x000000FF",
                "0xFFFFFFFF"
            ],
            "positions": [
                0.0,
                1.0
            ],
            "midpoints": [
                0.5
            ],
            "blur_width": 1.0,
            "color_space": 0,
            "interpolation_path": 0
        }
    ]
}
)";

private:
    // ファイルパス
    std::filesystem::path m_preset_path;
    std::filesystem::path m_history_path;

    // 共通のファイルI/O
    template <typename T>
    ConfigLoadResult<T> loadConfigFile(const std::filesystem::path& path, const T& default_cfg) const
    {
        ConfigLoadResult<T> result{default_cfg, {}};

        std::ifstream ifs(path);
        if (!ifs) {
            result.error = "config file open failed: " + path.string();
            return result;
        }

        try {
            nlohmann::ordered_json j = nlohmann::ordered_json::parse(ifs);
            result.config = j.get<T>();
        } catch (const nlohmann::json::exception& e) {
            result.error  = e.what();
            result.config = default_cfg;
        } catch (const std::exception& e) {
            result.error  = e.what();
            result.config = default_cfg;
        } catch (...) {
            result.error  = "unknown error";
            result.config = default_cfg;
        }

        return result;
    }

    template <typename T>
    ConfigWriteResult writeConfigFile(const T& cfg, const std::filesystem::path& path)
    {
        std::ofstream ofs(path);
        if (!ofs) {
            return {false, "failed to open config file: " + path.string()};
        }

        try {
            nlohmann::ordered_json j = cfg;
            ofs << j.dump(4);
        } catch (const nlohmann::json::exception& e) {
            return {false, e.what()};
        } catch (const std::exception& e) {
            return {false, e.what()};
        } catch (...) {
            return {false, "unknown error"};
        }

        return {true, {}};
    }

public:
    GradientConfigManager() = default;
    GradientConfigManager(
        const std::filesystem::path& preset_path,
        const std::filesystem::path& history_path)
        : m_preset_path{preset_path}
        , m_history_path{history_path}
    {}

    void setPresetFilePath(const std::filesystem::path& path)  { m_preset_path  = path; }
    void setHistoryFilePath(const std::filesystem::path& path) { m_history_path = path; }

    ConfigLoadResult<PresetConfig> loadPresetConfig() const
    {
        return loadConfigFile<PresetConfig>(m_preset_path, PresetConfig{});
    }

    ConfigLoadResult<HistoryConfig> loadHistoryConfig() const
    {
        return loadConfigFile<HistoryConfig>(m_history_path, HistoryConfig{});
    }

    // データ変換
    static GradientData preset2gradient(const GradientPreset& preset)
    {
        GradientData gradient{};
        std::vector<GradientMarkerData> markers_data;
        for (uint32_t i = 0; i < static_cast<uint32_t>(std::ssize(preset.colors)); ++i) {
            GradientMarkerData marker_data;
            marker_data.color = color_conv::u32Rgba2Vec4Rgba<ImVec4>(str_conv::charsToInt(preset.colors[i].substr(2, 8), 0xffffffff, 16));
            marker_data.pos   = preset.positions[i];
            if (static_cast<int32_t>(i) < static_cast<int32_t>(std::ssize(preset.colors)) - 1) {
                marker_data.midpoint.ratio = preset.midpoints[i];
            }
            markers_data.push_back(marker_data);
        }

        std::vector<AlphaMarkerData> alpha_markers_data;
        for (uint32_t i = 0; i < static_cast<uint32_t>(std::ssize(preset.alpha_values)); ++i) {
            AlphaMarkerData alpha_marker_data;
            alpha_marker_data.value = preset.alpha_values[i];
            alpha_marker_data.pos   = preset.alpha_positions[i];
            alpha_markers_data.push_back(alpha_marker_data);
        }

        gradient.m_marker_manager.setDefaultMarkers(markers_data);
        gradient.m_marker_manager.setDefaultAlphaMarkers(alpha_markers_data);
        gradient.m_blur_width  = preset.blur_width;
        gradient.m_color_space = preset.color_space;
        gradient.m_interp_dir  = preset.interpolation_path;
        return gradient;
    }

    static GradientPreset gradient2preset(GradientData& gradient)
    {
        GradientPreset preset;
        std::vector<std::string> rgba_hex_strs(static_cast<uint32_t>(std::ssize(gradient.m_marker_manager.getMarkerColors())));
        for (const auto& [i, marker_color] : gradient.m_marker_manager.getMarkerColors() | std::views::enumerate) {
            uint32_t rgba    = color_conv::vec4Rgba2u32Rgba<ImVec4>(marker_color);
            rgba_hex_strs[i] = std::format("0x{:08X}", rgba);
        }
        preset.colors             = rgba_hex_strs;
        preset.positions          = gradient.m_marker_manager.getMarkerPos();
        preset.midpoints          = gradient.m_marker_manager.getMidpointRatios();
        preset.alpha_values       = gradient.m_marker_manager.getAlphaMarkerValues();
        preset.alpha_positions    = gradient.m_marker_manager.getAlphaMarkerPos();
        preset.blur_width         = gradient.getBlurWidth();
        preset.color_space        = gradient.getColorSpace();
        preset.interpolation_path = gradient.getInterpDir();
        return preset;
    }

    static GradientData history2gradient(const GradientHistory& history)
    {
        GradientData gradient{};
        std::vector<GradientMarkerData> markers_data;
        for (uint32_t i = 0; i < static_cast<uint32_t>(std::ssize(history.colors)); ++i) {
            GradientMarkerData marker_data;
            marker_data.color = color_conv::u32Rgba2Vec4Rgba<ImVec4>(str_conv::charsToInt(history.colors[i].substr(2, 8), 0xffffffff, 16));
            marker_data.pos   = history.positions[i];
            if (static_cast<int32_t>(i) < static_cast<int32_t>(std::ssize(history.colors)) - 1) {
                marker_data.midpoint.ratio = history.midpoints[i];
            }
            markers_data.push_back(marker_data);
        }

        std::vector<AlphaMarkerData> alpha_markers_data;
        for (uint32_t i = 0; i < static_cast<uint32_t>(std::ssize(history.alpha_values)); ++i) {
            AlphaMarkerData alpha_marker_data;
            alpha_marker_data.value = history.alpha_values[i];
            alpha_marker_data.pos   = history.alpha_positions[i];
            alpha_markers_data.push_back(alpha_marker_data);
        }

        gradient.m_marker_manager.setDefaultMarkers(markers_data);
        gradient.m_marker_manager.setDefaultAlphaMarkers(alpha_markers_data);
        gradient.m_blur_width  = history.blur_width;
        gradient.m_color_space = history.color_space;
        gradient.m_interp_dir  = history.interpolation_path;
        return gradient;
    }

    static GradientHistory gradient2history(GradientData& gradient)
    {
        GradientHistory history;
        std::vector<std::string> rgba_hex_strs(static_cast<uint32_t>(std::ssize(gradient.m_marker_manager.getMarkerColors())));
        for (const auto& [i, marker_color] : gradient.m_marker_manager.getMarkerColors() | std::views::enumerate) {
            uint32_t rgba    = color_conv::vec4Rgba2u32Rgba<ImVec4>(marker_color);
            rgba_hex_strs[i] = std::format("0x{:08X}", rgba);
        }
        history.colors             = rgba_hex_strs;
        history.positions          = gradient.m_marker_manager.getMarkerPos();
        history.midpoints          = gradient.m_marker_manager.getMidpointRatios();
        history.alpha_values       = gradient.m_marker_manager.getAlphaMarkerValues();
        history.alpha_positions    = gradient.m_marker_manager.getAlphaMarkerPos();
        history.blur_width         = gradient.getBlurWidth();
        history.color_space        = gradient.getColorSpace();
        history.interpolation_path = gradient.getInterpDir();
        return history;
    }

    // プリセット操作
    ConfigWriteResult writePreset(const PresetConfig& cfg)
    {
        return writeConfigFile(cfg, m_preset_path);
    }

    static bool containsCategory(const std::vector<std::string>& categories, std::string_view target)
    {
        return std::ranges::contains(categories, target);
    }

    ConfigWriteResult addPreset(PresetConfig& cfg, GradientPreset preset,
                                std::string_view name, std::string_view category)
    {
        // 名前の重複を "_copy" サフィックスで回避
        std::string candidate{name};
        while (std::ranges::any_of(cfg.presets, [&](const auto& p){ return p.name == candidate; })) {
            candidate += "_copy";
        }
        preset.name     = candidate;
        preset.category = category;
        cfg.presets.push_back(preset);
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult deletePreset(PresetConfig& cfg, uint32_t index)
    {
        if (index >= static_cast<uint32_t>(std::ssize(cfg.presets))) {
            return {false, "The specified preset index is invalid."};
        }
        cfg.presets.erase(cfg.presets.begin() + index);
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult overwritePreset(PresetConfig& cfg, GradientPreset preset,
                                      uint32_t index, std::string_view name, std::string_view category)
    {
        if (index >= static_cast<uint32_t>(std::ssize(cfg.presets))) {
            return {false, "The specified preset index is invalid."};
        }
        preset.name           = name;
        preset.category       = category;
        cfg.presets[index]    = preset;
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult swapPreset(PresetConfig& cfg, uint32_t a, uint32_t b)
    {
        if (a >= static_cast<uint32_t>(std::ssize(cfg.presets)) ||
            b >= static_cast<uint32_t>(std::ssize(cfg.presets))) {
            return {false, "The specified preset index is invalid."};
        }
        std::swap(cfg.presets[a], cfg.presets[b]);
        return writeConfigFile(cfg, m_preset_path);
    }

    // カテゴリー操作
    ConfigWriteResult addCategory(PresetConfig& cfg, std::string_view name)
    {
        cfg.categories.push_back(name.data());
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult changeCategory(PresetConfig& cfg, uint32_t preset_index, std::string_view category)
    {
        if (preset_index >= static_cast<uint32_t>(std::ssize(cfg.presets))) {
            return {false, "The specified preset index is invalid."};
        }
        cfg.presets[preset_index].category = category;
        return writeConfigFile(cfg, m_preset_path);
    }

    // 全プリセットのカテゴリー a を b に置換
    ConfigWriteResult changeCategory(PresetConfig& cfg, std::string_view a, std::string_view b)
    {
        for (auto& p : cfg.presets) {
            if (p.category == a) p.category = b;
        }
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult changeCategories(PresetConfig& cfg, const std::vector<std::string>& new_categories)
    {
        cfg.categories = new_categories;
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult swapCategory(PresetConfig& cfg, uint32_t a, uint32_t b)
    {
        std::swap(cfg.categories[a], cfg.categories[b]);
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult deleteOnlyCategory(PresetConfig& cfg, std::string_view target)
    {
        std::erase(cfg.categories, std::string{target});
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult deleteCategoryAndPresets(PresetConfig& cfg, std::string_view target)
    {
        std::erase(cfg.categories, std::string{target});
        std::erase_if(cfg.presets, [&](const auto& p){ return p.category == target; });
        return writeConfigFile(cfg, m_preset_path);
    }

    // 履歴操作
    ConfigWriteResult writeHistory(const HistoryConfig& cfg)
    {
        return writeConfigFile(cfg, m_history_path);
    }

    ConfigWriteResult addHistory(HistoryConfig& cfg, GradientHistory history)
    {
        cfg.histories.push_back(std::move(history));
        return writeConfigFile(cfg, m_history_path);
    }

    ConfigWriteResult deleteHistory(HistoryConfig& cfg)
    {
        cfg.histories.clear();
        return writeConfigFile(cfg, m_history_path);
    }
};

#endif  // !GRADIENT_CONFIG_H
