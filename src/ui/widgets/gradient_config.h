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
    T config{};
    std::string error{};  // 空なら成功
    bool is_success() const { return error.empty(); }
};

struct ConfigWriteResult {
    bool is_success;
    std::string error;  // 空なら成功
};

// 旧形式（v0.4.2 まで）
struct OldGradientPreset {
    std::string category{"Uncategorized"};
    std::string name{"default"};
    std::vector<std::string> colors{"0x000000ff", "0xffffffff"};
    std::vector<float> positions{0.0f, 1.0f};
    std::vector<float> midpoints{0.5f};
    float blur_width{1.0f};
    int32_t color_space{0};
    int32_t interpolation_path{0};
};

inline void to_json(nlohmann::ordered_json& j, const OldGradientPreset& preset)
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

inline void from_json(const nlohmann::ordered_json& j, OldGradientPreset& preset)
{
    preset.category           = j.value("category", preset.category);
    preset.name               = j.value("name", preset.name);
    preset.colors             = j.value("colors", preset.colors);
    preset.positions          = j.value("positions", preset.positions);
    preset.midpoints          = j.value("midpoints", preset.midpoints);
    preset.blur_width         = j.value("blur_width", preset.blur_width);
    preset.color_space        = j.value("color_space", preset.color_space);
    preset.interpolation_path = j.value("interpolation_path", preset.interpolation_path);
}

// 新形式（v0.5.0 以降）
struct ColorMarker {
    std::string color{"0x000000ff"};
    float position{0};
    float midpoint{0.5f};
};

inline void to_json(nlohmann::ordered_json& j, const ColorMarker& c)
{
    j = nlohmann::ordered_json{
        {"color", c.color},
        {"position", c.position},
        {"midpoint", c.midpoint},
    };
}

inline void from_json(const nlohmann::ordered_json& j, ColorMarker& c)
{
    c.color    = j.value("color", c.color);
    c.position = j.value("position", c.position);
    c.midpoint = j.value("midpoint", c.midpoint);
}

struct AlphaMarker {
    float value{0};
    float position{0};
    float midpoint{0.5f};
};

inline void to_json(nlohmann::ordered_json& j, const AlphaMarker& c)
{
    j = nlohmann::ordered_json{
        {"value", c.value},
        {"position", c.position},
        {"midpoint", c.midpoint},
    };
}

inline void from_json(const nlohmann::ordered_json& j, AlphaMarker& c)
{
    c.value = j.value("value", c.value);
    c.position = j.value("position", c.position);
    c.midpoint = j.value("midpoint", c.midpoint);
}

struct GradientPreset {
    std::string category{"Uncategorized"};
    std::string name{"default"};
    std::vector<ColorMarker> color_markers{{"0x000000ff", 0.0f, 0.5f}, {"0x000000ff", 1.0f, 0.5f}};
    float color_blur_width{1.0f};
    int32_t color_space{0};
    int32_t interpolation_path{0};
    std::vector<AlphaMarker> alpha_markers{{1.0f, 0.0f, 0.5f}, {1.0f, 1.0f, 0.5f}};
    float alpha_blur_width{1.0f};
};

inline void to_json(nlohmann::ordered_json& j, const GradientPreset& c)
{
    j = nlohmann::ordered_json{
        {"category", c.category},
        {"name", c.name},
        {"color_markers", c.color_markers},
        {"color_blur_width", c.color_blur_width},
        {"color_space", c.color_space},
        {"interpolation_path", c.interpolation_path},
        {"alpha_markers", c.alpha_markers},
        {"alpha_blur_width", c.alpha_blur_width},
    };
}

inline void from_json(const nlohmann::ordered_json& j, GradientPreset& c)
{
    c.category           = j.value("category", c.category);
    c.name               = j.value("name", c.name);
    c.color_markers      = j.value("color_markers", c.color_markers);
    c.color_blur_width   = j.value("color_blur_width", c.color_blur_width);
    c.color_space        = j.value("color_space", c.color_space);
    c.interpolation_path = j.value("interpolation_path", c.interpolation_path);
    c.alpha_markers      = j.value("alpha_markers", c.alpha_markers);
    c.alpha_blur_width   = j.value("alpha_blur_width", c.alpha_blur_width);
}

// 旧形式（v0.4.2 まで）
struct OldGradientHistory {
    std::string name{"default"};
    std::vector<std::string> colors{"0x000000ff", "0xffffffff"};
    std::vector<float> positions{0.0f, 1.0f};
    std::vector<float> midpoints{0.5f};
    float blur_width{1.0f};
    int32_t color_space{0};
    int32_t interpolation_path{0};

    auto operator<=>(const OldGradientHistory&) const = default;
};

inline void to_json(nlohmann::ordered_json& j, const OldGradientHistory& history)
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

inline void from_json(const nlohmann::ordered_json& j, OldGradientHistory& history)
{
    history.name               = j.value("name", history.name);
    history.colors             = j.value("colors", history.colors);
    history.positions          = j.value("positions", history.positions);
    history.midpoints          = j.value("midpoints", history.midpoints);
    history.blur_width         = j.value("blur_width", history.blur_width);
    history.color_space        = j.value("color_space", history.color_space);
    history.interpolation_path = j.value("interpolation_path", history.interpolation_path);
}

// 新形式（v0.5.0 以降）
struct GradientHistory {
    std::string name{"default"};
    std::vector<ColorMarker> color_markers{{"0x000000ff", 0.0f, 0.5f}, {"0x000000ff", 1.0f, 0.5f}};
    float color_blur_width{1.0f};
    int32_t color_space{0};
    int32_t interpolation_path{0};
    std::vector<AlphaMarker> alpha_markers{{1.0f, 0.0f, 0.5f}, {1.0f, 1.0f, 0.5f}};
    float alpha_blur_width{1.0f};
};

inline void to_json(nlohmann::ordered_json& j, const GradientHistory& c)
{
    j = nlohmann::ordered_json{
        {"name", c.name},
        {"color_markers", c.color_markers},
        {"color_blur_width", c.color_blur_width},
        {"color_space", c.color_space},
        {"interpolation_path", c.interpolation_path},
        {"alpha_markers", c.alpha_markers},
        {"alpha_blur_width", c.alpha_blur_width},
    };
}

inline void from_json(const nlohmann::ordered_json& j, GradientHistory& c)
{
    c.name               = j.value("name", c.name);
    c.color_markers      = j.value("color_markers", c.color_markers);
    c.color_blur_width   = j.value("color_blur_width", c.color_blur_width);
    c.color_space        = j.value("color_space", c.color_space);
    c.interpolation_path = j.value("interpolation_path", c.interpolation_path);
    c.alpha_markers      = j.value("alpha_markers", c.alpha_markers);
    c.alpha_blur_width   = j.value("alpha_blur_width", c.alpha_blur_width);
}

// 旧形式（v0.4.2 まで）
struct PresetConfig {
    int32_t selected_category = 0;
    std::vector<std::string> categories{"Uncategorized"};
    std::vector<OldGradientPreset> presets{OldGradientPreset{}};
};

inline void to_json(nlohmann::ordered_json& j, const PresetConfig& cfg)
{
    j = nlohmann::ordered_json{
        {"selected_category", cfg.selected_category},
        {"categories", cfg.categories.empty() ? std::vector<std::string>{"Uncategorized"} : cfg.categories},
        {"presets", cfg.presets.empty() ? std::vector<OldGradientPreset>{OldGradientPreset{}} : cfg.presets}};
}

inline void from_json(const nlohmann::ordered_json& j, PresetConfig& cfg)
{
    cfg.selected_category = j.value("selected_category", cfg.selected_category);
    cfg.categories        = j.value("categories", cfg.categories);
    cfg.presets           = j.value("presets", cfg.presets);
}

// 新形式（v0.5.0 以降）
struct Preset {
    std::string version{"0.5.0"};
    int32_t selected_category = 0;
    std::vector<std::string> categories{"Uncategorized"};
    std::vector<GradientPreset> presets{GradientPreset{}};
};

inline void to_json(nlohmann::ordered_json& j, const Preset& c)
{
    j = nlohmann::ordered_json{
        {"version", c.version},
        {"selected_category", c.selected_category},
        {"categories", c.categories},
        {"presets", c.presets},
    };
}

inline void from_json(const nlohmann::ordered_json& j, Preset& c)
{
    // バージョン、選択カテゴリー、カテゴリーの配列は新旧共通項目
    c.version           = j.value("version", "0.0.0");
    c.selected_category = j.value("selected_category", c.selected_category);
    c.categories = j.value("categories", c.categories);

    if (c.version == "0.0.0") {
        // 旧形式で読み込んでから新形式の構造に変換する
        std::vector<OldGradientPreset> old_presets_{OldGradientPreset{}};
        auto old_presets = j.value("presets", old_presets_);
        std::vector<GradientPreset> new_preset;

        for (const auto& op : old_presets) {
            GradientPreset np;
            np.category           = op.category;
            np.name               = op.name;
            np.color_space        = op.color_space;
            np.color_blur_width   = op.blur_width;
            np.alpha_blur_width   = 1.0;
            np.interpolation_path = op.interpolation_path;

            // カラーマーカー
            int32_t color_marker_num = static_cast<int32_t>(std::ssize(op.colors));
            std::vector<ColorMarker> color_markers(color_marker_num);
            for (int32_t i = 0; i < color_marker_num; ++i) {
                ColorMarker color_marker;
                color_marker.color    = op.colors[i];
                color_marker.position = op.positions[i];
                if (i < color_marker_num - 1) {
                    color_marker.midpoint = op.midpoints[i];
                } else {
                    color_marker.midpoint = 0.5f;
                }
                color_markers[i] = color_marker;
            }
            np.color_markers = color_markers;

            // アルファマーカー
            std::vector<AlphaMarker> alpha_markers(2);
            alpha_markers[0] = {1.0f, 0.0f, 0.5f};
            alpha_markers[1] = {1.0f, 1.0f, 0.5f};
            np.alpha_markers = alpha_markers;

            new_preset.push_back(np);
        }
        c.presets = new_preset;
        c.version = "0.5.0";  // 書き込み時は v0.5.0 の形式にする
    } else if (c.version == "0.5.0") {
        c.presets = j.value("presets", c.presets);
    }
}

// 旧形式（v0.4.2 まで）
struct HistoryConfig {
    std::vector<OldGradientHistory> histories{};
};

inline void to_json(nlohmann::ordered_json& j, const HistoryConfig& cfg)
{
    j = nlohmann::ordered_json{{"histories", cfg.histories}};
}

inline void from_json(const nlohmann::ordered_json& j, HistoryConfig& cfg)
{
    cfg.histories = j.value("histories", cfg.histories);
}

// 新形式（v0.5.0 以降）
struct History {
    std::string version;
    std::vector<GradientHistory> histories{};
};

inline void to_json(nlohmann::ordered_json& j, const History& c)
{
    j = nlohmann::ordered_json{
        {"version", c.version},
        {"histories", c.histories}};
}

inline void from_json(const nlohmann::ordered_json& j, History& c)
{
    c.version = j.value("version", "0.0.0");

    if (c.version == "0.0.0") {
        // 旧形式で読み込んでから新形式の構造に変換する
        std::vector<OldGradientHistory> old_histories_{OldGradientHistory{}};
        auto old_histories = j.value("histories", old_histories_);
        std::vector<GradientHistory> new_histories;

        for (const auto& op : old_histories) {
            GradientHistory nh;
            nh.name               = op.name;
            nh.color_space        = op.color_space;
            nh.color_blur_width   = op.blur_width;
            nh.alpha_blur_width   = 1.0;
            nh.interpolation_path = op.interpolation_path;

            // カラーマーカー
            int32_t color_marker_num = static_cast<int32_t>(std::ssize(op.colors));
            std::vector<ColorMarker> color_markers(color_marker_num);
            for (int32_t i = 0; i < color_marker_num; ++i) {
                ColorMarker color_marker;
                color_marker.color    = op.colors[i];
                color_marker.position = op.positions[i];
                if (i < color_marker_num - 1) {
                    color_marker.midpoint = op.midpoints[i];
                } else {
                    color_marker.midpoint = 0.5f;
                }
                color_markers[i] = color_marker;
            }
            nh.color_markers = color_markers;

            // アルファマーカー
            std::vector<AlphaMarker> alpha_markers(2);
            alpha_markers[0] = {1.0f, 0.0f, 0.5f};
            alpha_markers[1] = {1.0f, 1.0f, 0.5f};
            nh.alpha_markers = alpha_markers;

            new_histories.push_back(nh);
        }
        c.histories = new_histories;
        c.version = "0.5.0";  // 書き込み時は v0.5.0 の形式にする
    } else if (c.version == "0.5.0") {
        c.histories = j.value("histories", c.histories);
    }
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
        : m_preset_path{preset_path}, m_history_path{history_path}
    {
    }

    void setPresetFilePath(const std::filesystem::path& path) { m_preset_path = path; }
    void setHistoryFilePath(const std::filesystem::path& path) { m_history_path = path; }

    ConfigLoadResult<Preset> loadPresetConfig() const
    {
        return loadConfigFile<Preset>(m_preset_path, Preset{});
    }

    ConfigLoadResult<History> loadHistoryConfig() const
    {
        return loadConfigFile<History>(m_history_path, History{});
    }

    static GradientData preset2gradient(const GradientPreset& preset)
    {
        GradientData gradient{};

        // カラーマーカー
        std::vector<GradientMarkerData> color_markers;
        for (uint32_t i = 0; i < static_cast<uint32_t>(std::ssize(preset.color_markers)); ++i) {
            GradientMarkerData marker_data;
            marker_data.color          = color_conv::u32Rgba2Vec4Rgba<ImVec4>(str_conv::charsToInt(preset.color_markers[i].color.substr(2, 8), 0xffffffff, 16));
            marker_data.pos            = preset.color_markers[i].position;
            marker_data.midpoint.ratio = preset.color_markers[i].midpoint;
            color_markers.push_back(marker_data);
        }

        // アルファマーカー
        std::vector<AlphaMarkerData> alpha_markers;
        for (uint32_t i = 0; i < static_cast<uint32_t>(std::ssize(preset.alpha_markers)); ++i) {
            AlphaMarkerData alpha_marker_data;
            alpha_marker_data.value          = preset.alpha_markers[i].value;
            alpha_marker_data.pos            = preset.alpha_markers[i].position;
            alpha_marker_data.midpoint.ratio = preset.alpha_markers[i].midpoint;
            alpha_markers.push_back(alpha_marker_data);
        }

        gradient.m_marker_manager.setDefaultMarkers(color_markers);
        gradient.m_marker_manager.setDefaultAlphaMarkers(alpha_markers);
        gradient.m_blur_width       = preset.color_blur_width;
        gradient.m_alpha_blur_width = preset.alpha_blur_width;
        gradient.m_color_space      = preset.color_space;
        gradient.m_interp_dir       = preset.interpolation_path;

        return gradient;
    }

    static GradientPreset gradient2preset(GradientData& gradient)
    {
        GradientPreset preset;

        // カラーマーカー
        int32_t color_marker_num = static_cast<int32_t>(std::ssize(gradient.m_marker_manager.getMarkerPos()));
        std::vector<ColorMarker> color_markers(color_marker_num);
        std::vector<ImVec4> color_marker_colors   = gradient.m_marker_manager.getMarkerColors();
        auto color_marker_positions = gradient.m_marker_manager.getMarkerPos();
        auto color_marker_midpoints = gradient.m_marker_manager.getMidpointRatios();
        for (int32_t i = 0; i < color_marker_num; ++i) {
            uint32_t rgba              = color_conv::vec4Rgba2u32Rgba<ImVec4>(color_marker_colors[i]);
            color_markers[i].color    = std::format("0x{:08X}", rgba);
            color_markers[i].position = color_marker_positions[i];
            if (i < color_marker_num - 1) {
                color_markers[i].midpoint = color_marker_midpoints[i];
            } else {
                color_markers[i].midpoint = 0.5f;
            }
        }

        // アルファマーカー
        int32_t alpha_marker_num = static_cast<int32_t>(std::ssize(gradient.m_marker_manager.getAlphaMarkerPos()));
        std::vector<AlphaMarker> alpha_markers(alpha_marker_num);
        auto alpha_marker_values     = gradient.m_marker_manager.getAlphaMarkerValues();
        auto alpha_marker_positionts = gradient.m_marker_manager.getAlphaMarkerPos();
        auto alpha_marker_midpoints  = gradient.m_marker_manager.getAlphaMidpointRatios();
        for (int32_t i = 0; i < alpha_marker_num; ++i) {
            alpha_markers[i].value = alpha_marker_values[i];
            alpha_markers[i].position = alpha_marker_positionts[i];
            if (i < alpha_marker_num - 1) {
                alpha_markers[i].midpoint = alpha_marker_midpoints[i];
            } else {
                alpha_markers[i].midpoint = 0.5f;
            }
        }

        preset.color_markers      = color_markers;
        preset.alpha_markers      = alpha_markers;
        preset.color_blur_width   = gradient.getBlurWidth();
        preset.alpha_blur_width   = gradient.getAlphaBlurWidth();
        preset.color_space        = gradient.getColorSpace();
        preset.interpolation_path = gradient.getInterpDir();

        return preset;
    }

    static GradientData history2gradient(const GradientHistory& history)
    {
        GradientData gradient{};

        // カラーマーカー
        std::vector<GradientMarkerData> color_markers;
        for (uint32_t i = 0; i < static_cast<uint32_t>(std::ssize(history.color_markers)); ++i) {
            GradientMarkerData marker_data;
            marker_data.color          = color_conv::u32Rgba2Vec4Rgba<ImVec4>(str_conv::charsToInt(history.color_markers[i].color.substr(2, 8), 0xffffffff, 16));
            marker_data.pos            = history.color_markers[i].position;
            marker_data.midpoint.ratio = history.color_markers[i].midpoint;
            color_markers.push_back(marker_data);
        }

        // アルファマーカー
        std::vector<AlphaMarkerData> alpha_markers;
        for (uint32_t i = 0; i < static_cast<uint32_t>(std::ssize(history.alpha_markers)); ++i) {
            AlphaMarkerData alpha_marker_data;
            alpha_marker_data.value          = history.alpha_markers[i].value;
            alpha_marker_data.pos            = history.alpha_markers[i].position;
            alpha_marker_data.midpoint.ratio = history.alpha_markers[i].midpoint;
            alpha_markers.push_back(alpha_marker_data);
        }

        gradient.m_marker_manager.setDefaultMarkers(color_markers);
        gradient.m_marker_manager.setDefaultAlphaMarkers(alpha_markers);
        gradient.m_blur_width       = history.color_blur_width;
        gradient.m_alpha_blur_width = history.alpha_blur_width;
        gradient.m_color_space      = history.color_space;
        gradient.m_interp_dir       = history.interpolation_path;
        return gradient;
    }

    static GradientHistory gradient2history(GradientData& gradient)
    {
        GradientHistory history;

        // カラーマーカー
        int32_t color_marker_num = static_cast<int32_t>(std::ssize(gradient.m_marker_manager.getMarkerPos()));
        std::vector<ColorMarker> color_markers(color_marker_num);
        std::vector<ImVec4> color_marker_colors   = gradient.m_marker_manager.getMarkerColors();
        auto color_marker_positions = gradient.m_marker_manager.getMarkerPos();
        auto color_marker_midpoints = gradient.m_marker_manager.getMidpointRatios();
        for (int32_t i = 0; i < color_marker_num; ++i) {
            uint32_t rgba              = color_conv::vec4Rgba2u32Rgba<ImVec4>(color_marker_colors[i]);
            color_markers[i].color    = std::format("0x{:08X}", rgba);
            color_markers[i].position = color_marker_positions[i];
            if (i < color_marker_num - 1) {
                color_markers[i].midpoint = color_marker_midpoints[i];
            } else {
                color_markers[i].midpoint = 0.5f;
            }
        }

        // アルファマーカー
        int32_t alpha_marker_num = static_cast<int32_t>(std::ssize(gradient.m_marker_manager.getAlphaMarkerPos()));
        std::vector<AlphaMarker> alpha_markers(alpha_marker_num);
        auto alpha_marker_values     = gradient.m_marker_manager.getAlphaMarkerValues();
        auto alpha_marker_positionts = gradient.m_marker_manager.getAlphaMarkerPos();
        auto alpha_marker_midpoints  = gradient.m_marker_manager.getAlphaMidpointRatios();
        for (int32_t i = 0; i < alpha_marker_num; ++i) {
            alpha_markers[i].value = alpha_marker_values[i];
            alpha_markers[i].position = alpha_marker_positionts[i];
            if (i < alpha_marker_num - 1) {
                alpha_markers[i].midpoint = alpha_marker_midpoints[i];
            } else {
                alpha_markers[i].midpoint = 0.5f;
            }
        }

        history.color_markers      = color_markers;
        history.alpha_markers      = alpha_markers;
        history.color_blur_width   = gradient.getBlurWidth();
        history.alpha_blur_width   = gradient.getAlphaBlurWidth();
        history.color_space        = gradient.getColorSpace();
        history.interpolation_path = gradient.getInterpDir();
        return history;
    }

    // プリセット操作
    ConfigWriteResult writePreset(const Preset& cfg)
    {
        return writeConfigFile(cfg, m_preset_path);
    }

    static bool containsCategory(const std::vector<std::string>& categories, std::string_view target)
    {
        return std::ranges::contains(categories, target);
    }

    ConfigWriteResult addPreset(Preset& cfg, GradientPreset preset,
                                std::string_view name, std::string_view category)
    {
        // 名前の重複を "_copy" サフィックスで回避
        std::string candidate{name};
        while (std::ranges::any_of(cfg.presets, [&](const auto& p) { return p.name == candidate; })) {
            candidate += "_copy";
        }
        preset.name     = candidate;
        preset.category = category;
        cfg.presets.push_back(preset);
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult deletePreset(Preset& cfg, uint32_t index)
    {
        if (index >= static_cast<uint32_t>(std::ssize(cfg.presets))) {
            return {false, "The specified preset index is invalid."};
        }
        cfg.presets.erase(cfg.presets.begin() + index);
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult overwritePreset(Preset& cfg, GradientPreset preset,
                                      uint32_t index, std::string_view name, std::string_view category)
    {
        if (index >= static_cast<uint32_t>(std::ssize(cfg.presets))) {
            return {false, "The specified preset index is invalid."};
        }
        preset.name        = name;
        preset.category    = category;
        cfg.presets[index] = preset;
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult swapPreset(Preset& cfg, uint32_t a, uint32_t b)
    {
        if (a >= static_cast<uint32_t>(std::ssize(cfg.presets)) ||
            b >= static_cast<uint32_t>(std::ssize(cfg.presets))) {
            return {false, "The specified preset index is invalid."};
        }
        std::swap(cfg.presets[a], cfg.presets[b]);
        return writeConfigFile(cfg, m_preset_path);
    }

    // カテゴリー操作
    ConfigWriteResult addCategory(Preset& cfg, std::string_view name)
    {
        cfg.categories.push_back(name.data());
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult changeCategory(Preset& cfg, uint32_t preset_index, std::string_view category)
    {
        if (preset_index >= static_cast<uint32_t>(std::ssize(cfg.presets))) {
            return {false, "The specified preset index is invalid."};
        }
        cfg.presets[preset_index].category = category;
        return writeConfigFile(cfg, m_preset_path);
    }

    // 全プリセットのカテゴリー a を b に置換
    ConfigWriteResult changeCategory(Preset& cfg, std::string_view a, std::string_view b)
    {
        for (auto& p : cfg.presets) {
            if (p.category == a) p.category = b;
        }
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult changeCategories(Preset& cfg, const std::vector<std::string>& new_categories)
    {
        cfg.categories = new_categories;
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult swapCategory(Preset& cfg, uint32_t a, uint32_t b)
    {
        std::swap(cfg.categories[a], cfg.categories[b]);
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult deleteOnlyCategory(Preset& cfg, std::string_view target)
    {
        std::erase(cfg.categories, std::string{target});
        return writeConfigFile(cfg, m_preset_path);
    }

    ConfigWriteResult deleteCategoryAndPresets(Preset& cfg, std::string_view target)
    {
        std::erase(cfg.categories, std::string{target});
        std::erase_if(cfg.presets, [&](const auto& p) { return p.category == target; });
        return writeConfigFile(cfg, m_preset_path);
    }

    // 履歴操作
    ConfigWriteResult writeHistory(const History& cfg)
    {
        return writeConfigFile(cfg, m_history_path);
    }

    ConfigWriteResult addHistory(History& cfg, GradientHistory history)
    {
        cfg.histories.push_back(std::move(history));
        return writeConfigFile(cfg, m_history_path);
    }

    ConfigWriteResult deleteHistory(History& cfg)
    {
        cfg.histories.clear();
        return writeConfigFile(cfg, m_history_path);
    }
};

#endif  // !GRADIENT_CONFIG_H
