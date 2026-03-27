#ifndef GRADIENT_CONFIG_H
#define GRADIENT_CONFIG_H

#include "gradient_data.h"
#include "color_conv.h"
#include "str_conv.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <compare>

#include "json.hpp"

struct GradientPreset {
    std::string category{"uncategorized"};
    std::string name{"default"};
    std::vector<std::string> colors{"0x000000ff", "0xffffffff"};
    std::vector<float> positions{0.0f, 1.0f};
    std::vector<float> midpoints{0.5f};
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
        {"blur_width", preset.blur_width},
        {"color_space", preset.color_space},
        {"interpolation_path", preset.interpolation_path}};
}

inline void from_json(const nlohmann::ordered_json& j, GradientPreset& preset)
{
    preset.category = j.value("category", preset.category);
    preset.name = j.value("name", preset.name);
    preset.colors = j.value("colors", preset.colors);
    preset.positions = j.value("positions", preset.positions);
    preset.midpoints = j.value("midpoints", preset.midpoints);
    preset.blur_width = j.value("blur_width", preset.blur_width);
    preset.color_space = j.value("color_space", preset.color_space);
    preset.interpolation_path = j.value("interpolation_path", preset.interpolation_path);
}

struct GradientHistory {
    std::string name{"default"};
    std::vector<std::string> colors{"0x000000ff", "0xffffffff"};
    std::vector<float> positions{0.0f, 1.0f};
    std::vector<float> midpoints{0.5f};
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
        {"blur_width", history.blur_width},
        {"color_space", history.color_space},
        {"interpolation_path", history.interpolation_path}};
}

inline void from_json(const nlohmann::ordered_json& j, GradientHistory& history)
{
    history.name = j.value("name", history.name);
    history.colors = j.value("colors", history.colors);
    history.positions = j.value("positions", history.positions);
    history.midpoints = j.value("midpoints", history.midpoints);
    history.blur_width = j.value("blur_width", history.blur_width);
    history.color_space = j.value("color_space", history.color_space);
    history.interpolation_path = j.value("interpolation_path", history.interpolation_path);
}

struct GradientConfig {
    std::vector<std::string> categories{"uncategorized"};
    std::vector<GradientPreset> presets{GradientPreset{}};
    std::vector<GradientHistory> histories{};
};

inline void to_json(nlohmann::ordered_json& j, const GradientConfig& config)
{
    auto categories = config.categories;
    if (categories.empty()) {
        categories = {"uncategorized"};
    }

    auto presets = config.presets;
    if (presets.empty()) {
        presets = {GradientPreset{}};
    }

    auto histories = config.histories;
    if (histories.empty()) {
        histories = std::vector<GradientHistory>{};
    }

    j = nlohmann::ordered_json{
        {"categories", categories},
        {"presets", presets},
        {"histories", histories}
    };
}

inline void from_json(const nlohmann::ordered_json& j, GradientConfig& config)
{
    config.categories = j.value("categories", config.categories);
    config.presets = j.value("presets", config.presets);
    config.histories = j.value("histories", std::vector<GradientHistory>{});
}

class GradientConfigManager
    : public GradientPreset,
      public GradientConfig {
public:
    inline static const char* DEFAULT_CATEGORY = "uncategorized";

private:
    const GradientConfig DEFAULT_PRESET_FILE{GradientConfig{
            .categories = { DEFAULT_CATEGORY },
            .presets = { GradientPreset{} }
        }};
    inline static const char* DEFAULT_PRESET_FILE_JSON = R"(
{
    "categories": [
        "uncategorized"
    ],
    "presets": [
        {
            "name": "black-white",
            "colors": [
                "0x000000FF",
                "0xFFFFFFFF"
            ],
            "positions": [
                0.00,
                1.00
            ],
            "midpoints": [
                0.50
            ],
            "blur_width": 1.0,
            "color_space": 0,
            "interpolation_path": 0
        },
        {
            "name": "black-transparent",
            "colors": [
                "0x000000FF",
                "0x00000000"
            ],
            "positions": [
                0.00,
                1.00
            ],
            "midpoints": [
                0.50
            ],
            "blur_width": 1.0,
            "color_space": 0,
            "interpolation_path": 0
        },
        {
            "name": "blue-yellow",
            "colors": [
                "0x0000FFFF",
                "0xFFFF00FF"
            ],
            "positions": [
                0.00,
                1.00
            ],
            "midpoints": [
                0.50
            ],
            "blur_width": 1.0,
            "color_space": 7,
            "interpolation_path": 0
        },
        {
            "name": "spectrum",
            "colors": [
                "0xFF0000FF",
                "0xFF00FFFF",
                "0x0000FFFF",
                "0x00FFFFFF",
                "0x00FF00FF",
                "0xFFFF00FF",
                "0xFF0000FF"
            ],
            "positions": [
                0.00,
                0.17,
                0.33,
                0.5,
                0.67,
                0.83,
                1.00
            ],
            "midpoints": [
                0.50,
                0.50,
                0.50,
                0.50,
                0.50,
                0.50
            ],
            "blur_width": 1.0,
            "color_space": 0,
            "interpolation_path": 0
        },
        {
            "name": "gold",
            "colors": [
                "0xAB7B01FF",
                "0xDBC06CFF",
                "0xFFFCACFF",
                "0x2E1C03FF",
                "0xAB7B01FF",
                "0xFFFCACFF"
            ],
            "positions": [
                0.00,
                0.17,
                0.30,
                0.40,
                0.80,
                1.00
            ],
            "midpoints": [
                0.60,
                0.60,
                0.90,
                0.50,
                0.50
            ],
            "blur_width": 1.0,
            "color_space": 1,
            "interpolation_path": 0
        }
    ],
    "histories": []
}
)";

    std::filesystem::path m_preset_path;

public:
    struct PresetLoadResult {
        GradientConfig preset_file;
        std::string error;  // 空なら成功
    };

    GradientConfigManager() = default;
    GradientConfigManager(const std::filesystem::path& path) : m_preset_path{path} {}

    void setPresetFilePath(const std::filesystem::path& path) { m_preset_path = path; }

    /// @brief プリセットファイル（json）を読み込む
    /// @return PresetLoadResult 構造体 @n 成功の場合は preset_file に読み込んだ値が入り、 error が空になる。 @n 失敗の場合は preset_file がデフォルト値になり、 error にエラーメッセージが入る。
    PresetLoadResult loadPresetFile() const
    {
        PresetLoadResult result;
        result.preset_file = DEFAULT_PRESET_FILE;

        std::ifstream ifs(m_preset_path);
        if (!ifs) {
            result.error = "preset file open failed";
            return result;
        }

        nlohmann::ordered_json j;
        try {
            j                  = nlohmann::ordered_json::parse(ifs);
            result.preset_file = j.get<GradientConfig>();
        } catch (const nlohmann::json::exception& e) {
            result.error = e.what();
            result.preset_file = DEFAULT_PRESET_FILE;
        } catch (const std::exception& e) {
            result.error = e.what();
            result.preset_file = DEFAULT_PRESET_FILE;
        }

        return result;
    };

    struct PresetWriteResult {
        bool is_success;
        std::string error;
    };
    /// @brief プリセットを指定されたファイル（json）に書き込む
    /// @param preset_file 書き込むプリセットの値
    /// @return PresetWriteResult 構造体 @n 成功の場合は is_success が true、error が空になる。 @n 失敗の場合は is_success が false、error にエラーメッセージが入る。
    PresetWriteResult writePresetFile(const GradientConfig& cfg)
    {
        PresetWriteResult result{false, {}};

        nlohmann::ordered_json j = cfg;
        std::ofstream ofs(m_preset_path);
        if (!ofs) {
            result.is_success = false;
            result.error = "failed to open preset file";
            return result;
        }

        try {
            std::string serialized_string = j.dump(4);
            ofs << serialized_string;
        } catch (const nlohmann::json::exception& e) {
            result.error = e.what();
            result.is_success = false;
            return result;
        } catch (const std::exception& e) {
            result.error = e.what();
            result.is_success = false;
            return result;
        }

        result.is_success = true;
        return result;
    }

    void createDefaultPresetFile(const std::filesystem::path& file_path)
    {
        std::ofstream ofs(file_path);
        ofs << DEFAULT_PRESET_FILE_JSON;
    }

    // 指定したカテゴリーがすでにに存在するか調べる
    static bool containsCategory(const std::vector<std::string>& categories, std::string_view target_category)
    {
        for (const auto& c : categories) {
            if (c == target_category) return true;
        }
        return false;
    }

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
        gradient.m_marker_manager.setDefaultMarkers(markers_data);
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
        gradient.m_marker_manager.setDefaultMarkers(markers_data);
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
        history.blur_width         = gradient.getBlurWidth();
        history.color_space        = gradient.getColorSpace();
        history.interpolation_path = gradient.getInterpDir();

        return history;
    }

    PresetWriteResult addPreset(GradientConfig& cfg, GradientPreset preset, std::string_view name, std::string_view category)
    {
        // 追加する前に名前の重複を避ける
        auto has_duplicate_name = [&cfg](std::string& target_name) {
            for (const auto& p : cfg.presets) {
                if (p.name == target_name) {
                    return true;
                }
            }
            return false;
        };

        std::string original_name  =  std::string{name};
        std::string candidate_name = original_name;
        while (has_duplicate_name(candidate_name)) {  // 重複がある限りループし続ける
            candidate_name += "_copy";
        }
        preset.name = candidate_name;
        preset.category = category;

        cfg.presets.push_back(preset);

        return writePresetFile(cfg);
    }

    PresetWriteResult deletePreset(GradientConfig& cfg, const uint32_t preset_index)
    {
        if (preset_index < 0 || static_cast<uint32_t>(std::ssize(cfg.presets)) <= preset_index) {
            return { false, "The specified preset index is invalid." };
        }

        cfg.presets.erase(cfg.presets.begin() + preset_index);

        return writePresetFile(cfg);
    }

    PresetWriteResult overwritePreset(GradientConfig& cfg, GradientPreset preset, const uint32_t preset_index, std::string_view name, std::string_view category)
    {
        if (preset_index < 0 || static_cast<uint32_t>(std::ssize(cfg.presets)) <= preset_index) {
            return { false, "The specified preset index is invalid." };
        }

        preset.name         = name;
        preset.category     = category;
        cfg.presets[preset_index] = preset;

        return writePresetFile(cfg);
    }

    PresetWriteResult swapPreset(GradientConfig& cfg, const uint32_t a, const uint32_t b)
    {
        if ((a < 0 || static_cast<uint32_t>(std::ssize(cfg.presets)) <= a) ||
            (b < 0 || static_cast<uint32_t>(std::ssize(cfg.presets)) <= b)) {
            return { false, "The specified preset index is invalid." };
        }

        auto tmp       = cfg.presets[a];
        cfg.presets[a] = cfg.presets[b];
        cfg.presets[b] = tmp;

        return writePresetFile(cfg);
    }

    PresetWriteResult addCategory(GradientConfig& cfg, std::string_view category)
    {
        for (const auto& c : cfg.categories) {
            if (c == category) {
                return { false, "A category with this name already exists." };
            }
        }
        cfg.categories.push_back(std::string{category});

        return writePresetFile(cfg);
    }

    // 指定したプリセットのカテゴリーを変更する
    PresetWriteResult changeCategory(GradientConfig& cfg, const uint32_t preset_index, std::string_view category)
    {
        if (preset_index < 0 || static_cast<uint32_t>(std::ssize(cfg.presets)) <= preset_index) {
            return { false, "The specified preset index is invalid." };
        }

        cfg.presets[preset_index].category = category;

        return writePresetFile(cfg);
    }

    // 指定したカテゴリーを別のカテゴリーに置き換える
    PresetWriteResult changeCategory(GradientConfig& cfg, std::string_view a, std::string_view b)
    {
        for (auto& preset : cfg.presets) {
            if (preset.category == a) {
                preset.category = b;
            }
        }

        return writePresetFile(cfg);
    }

    // カテゴリーの配列を置き換える
    PresetWriteResult changeCategories(GradientConfig& cfg, const std::vector<std::string>& categories)
    {
        cfg.categories = categories;

        return writePresetFile(cfg);
    }

    PresetWriteResult swapCategory(GradientConfig& cfg, const uint32_t a, const uint32_t b)
    {
        std::swap(cfg.categories[a], cfg.categories[b]);

        return writePresetFile(cfg);
    }

    PresetWriteResult deleteOnlyCategory(GradientConfig& cfg, std::string_view target_category)
    {
        auto it = std::find(cfg.categories.begin(), cfg.categories.end(), target_category);
        if (it != cfg.categories.end()) {
            cfg.categories.erase(it);
        }

        return writePresetFile(cfg);
    }

    // カテゴリーとそのカテゴリーに属する全てのプリセットを削除する
    PresetWriteResult deleteCategoryAndPresets(GradientConfig& cfg, std::string_view target_category)
    {
        // カテゴリーを削除
        auto it = std::find(cfg.categories.begin(), cfg.categories.end(), target_category);
        if (it != cfg.categories.end()) {
            cfg.categories.erase(it);
        }

        // target_category に属するプリセットを削除
        cfg.presets.erase(
            std::remove_if(cfg.presets.begin(), cfg.presets.end(),
            [&](const auto& p) { return p.category == target_category; }),
            cfg.presets.end()
        );

        return writePresetFile(cfg);
    }
};

#endif  // !GRADIENT_CONFIG_H
