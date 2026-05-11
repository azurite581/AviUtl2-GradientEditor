#ifndef GRADIENT_CONFIG_H
#define GRADIENT_CONFIG_H

#include <compare>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>
#include <utility>

#include "color_conv.h"
#include "gradient_data.h"
#include "gradient_marker.h"
#include "grd_codec.h"
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
    c.value    = j.value("value", c.value);
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
    c.categories        = j.value("categories", c.categories);

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
        c.version   = "0.5.0";  // 書き込み時は v0.5.0 の形式にする
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
            result.config            = j.get<T>();
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
        std::vector<ImVec4> color_marker_colors = gradient.m_marker_manager.getMarkerColors();
        auto color_marker_positions             = gradient.m_marker_manager.getMarkerPos();
        auto color_marker_midpoints             = gradient.m_marker_manager.getMidpointRatios();
        for (int32_t i = 0; i < color_marker_num; ++i) {
            uint32_t rgba             = color_conv::vec4normRgba2u32Rgba<ImVec4>(color_marker_colors[i]);
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
            alpha_markers[i].value    = alpha_marker_values[i];
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

    // Photoshop Gradient（GRD）から Gradient Editor のプリセット形式に変換
    static std::pair<std::vector<GradientPreset>, std::vector<std::string>> grd2preset(const GRD& grd, const std::string& category_name = "Uncategorized")
    {
        std::vector<GradientPreset> gradient_preset;
        std::vector<std::string> message;  // エラーメッセージなどを格納する

        for (const auto& grad : grd.gradient_list.gradient_list) {
            GradientPreset preset;
            preset.name     = grad.gradient_name;
            preset.category = category_name;

            // 対応する項目がないものは初期値にする
            preset.alpha_blur_width   = 1.0f;
            preset.color_space        = 0;
            preset.interpolation_path = 0;

            bool conversion_completed = true;
            if (grad.gradient_form == "CstS" && grad.gradient_object.index() == 0) {
                const auto& solid_grad = std::get<0>(grad.gradient_object);

                // ぼかし幅
                preset.color_blur_width = static_cast<float>(solid_grad.interpolation / 4096.0);

                // カラーマーカー
                int32_t color_marker_num = solid_grad.color_stops.item_num;
                std::vector<ColorMarker> color_markers;
                for (int32_t j = 0; j < color_marker_num; ++j) {
                    const auto& col = solid_grad.color_stops.color_stop_objects[j];

                    ColorMarker color_marker;
                    color_marker.position = static_cast<float>(col.location / 4096.0);
                    color_marker.midpoint = static_cast<float>(col.midpoint / 100.0);

                    // 色の形式に応じて変換
                    // using ColorObject = std::variant<BookColor, CMYC, Grsc, HSBC, LbCl, RGBC>;
                    int32_t color_type_index = col.color_object.color_object_variant.index();
                    switch (color_type_index) {
                        case 0:  // BookColor
                            message.push_back("BookColor stop is not supported.\n");
                            conversion_completed = false;
                            break;
                        case 1:  // CMYC
                            message.push_back("CMYK stop is not supported.\n");
                            conversion_completed = false;
                            break;
                        case 2:  // Grsc
                        {
                            const auto& gray    = std::get<2>(col.color_object.color_object_variant);
                            const auto u32_gray = static_cast<uint32_t>((gray.gray / 100.0) * 255.0f + 0.5f);
                            const auto u32_rgba = color_conv::vec4Rgba2u32Rgba<ImVec4>({static_cast<float>(u32_gray), static_cast<float>(u32_gray), static_cast<float>(u32_gray), 255.0f});
                            color_marker.color  = std::format("0x{:08X}", u32_rgba);
                            break;
                        }
                        case 3:  // HSBC
                        {
                            const auto& hsb        = std::get<3>(col.color_object.color_object_variant);
                            const auto norm_srgb   = color_conv::hsv2srgb({hsb.hue * std::numbers::pi / 180.0, hsb.saturate / 100.0, hsb.brightness / 100.0});
                            const ImVec4 norm_rgba = ImVec4{static_cast<float>(norm_srgb.r), static_cast<float>(norm_srgb.g), static_cast<float>(norm_srgb.b), 1.0f};
                            const auto u32_rgba    = color_conv::vec4normRgba2u32Rgba<ImVec4>(norm_rgba);
                            color_marker.color     = std::format("0x{:08X}", u32_rgba);
                            break;
                        }
                        case 4:  // LbCl
                        {
                            const auto& lab        = std::get<4>(col.color_object.color_object_variant);
                            const auto norm_srgb   = color_conv::d50lab2srgb({lab.luminance, lab.a, lab.b});
                            const ImVec4 norm_rgba = ImVec4{static_cast<float>(norm_srgb.r), static_cast<float>(norm_srgb.g), static_cast<float>(norm_srgb.b), 1.0f};
                            const auto u32_rgba    = color_conv::vec4normRgba2u32Rgba<ImVec4>(norm_rgba);
                            color_marker.color     = std::format("0x{:08X}", u32_rgba);
                            break;
                        }
                        case 5:  // RGBC
                        {
                            const auto& rgb     = std::get<5>(col.color_object.color_object_variant);
                            const ImVec4 rgba   = ImVec4{static_cast<float>(rgb.red), static_cast<float>(rgb.green), static_cast<float>(rgb.blue), 255.0f};
                            const auto u32_rgba = color_conv::vec4Rgba2u32Rgba<ImVec4>(rgba);
                            color_marker.color  = std::format("0x{:08X}", u32_rgba);
                            break;
                        }
                        default:
                        {
                            message.push_back("Unknown color type.\n");
                            conversion_completed = false;
                            break;
                        }
                    }
                    color_markers.push_back(color_marker);
                }

                if (!conversion_completed) {
                    break;
                }
                preset.color_markers = color_markers;

                // 不透明度マーカー
                std::vector<AlphaMarker> alpha_markers;
                for (const auto& transparency : solid_grad.transparency_stops.transparency_stop_objects) {
                    AlphaMarker alpha_marker;
                    alpha_marker.value    = static_cast<float>(transparency.opacity / 100.0);
                    alpha_marker.position = static_cast<float>(transparency.location / 4096.0);
                    alpha_marker.midpoint = static_cast<float>(transparency.midpoint / 100.0);
                    alpha_markers.push_back(alpha_marker);
                }
                preset.alpha_markers = alpha_markers;
            } else {
                // ノイズグラデーションは無視する
                message.push_back("Noise gradients are not supported.\n");
                conversion_completed = false;
                continue;
            }

            // 全ての変換が成功したときだけプリセットに追加
            if (conversion_completed) {
                gradient_preset.push_back(preset);
            }
        }

        auto result_log = std::format("Number of gradient presets loaded: {}\n", std::ssize(gradient_preset));
        message.push_back(result_log);

        return {gradient_preset, message};
    }

    // プリセット形式から GRD 形式に変換する
    static std::expected<GRD, std::string> presets2grd(const std::vector<GradientPreset>& gradient)
    {
        GRD grd;
        grd.header = {
            .signature = "8BGR",
            .version = 5,
            .descriptor_version = 16,
        };

        grd.descripter_object = {
            .key_item_num = 1
        };

        std::vector<Gradient> gradient_list;
        for (const auto& g : gradient) {
            Gradient grd_gradient;
            grd_gradient.gradient_name = g.name;
            grd_gradient.gradient_form = "CstS";  // カスタムストップグラデーション形式

            // CustomStopsGradientObject = {interpolation, color_stops, transparency_stops}
            // interpolation
            CustomStopsGradientObject grd_gradient_object;
            grd_gradient_object.interpolation = g.color_blur_width * 4096;

            // color_stops
            ColorStops color_stops;
            color_stops.item_num = static_cast<int32_t>(std::ssize(g.color_markers));
            std::vector<ColorStopObject> color_stop_object(color_stops.item_num);
            for (int32_t j = 0; auto& cso : color_stop_object) {
                cso.type = "UsrS";  // ユーザーストップ
                cso.location = static_cast<int32_t>(g.color_markers[j].position * 4096.0);
                cso.midpoint = static_cast<int32_t>(g.color_markers[j].midpoint * 100.0);

                RGBC rgbc;
                uint32_t t = std::stoul(g.color_markers[j].color, nullptr, 16);
                auto rgba = color_conv::hexRgba2rgba(t);
                rgbc.red = rgba.r;
                rgbc.green = rgba.g;
                rgbc.blue = rgba.b;

                cso.color_object.color_type = "RGBC";
                cso.color_object.color_object_variant = rgbc;

                ++j;
            }
            color_stops.color_stop_objects = color_stop_object;
            grd_gradient_object.color_stops = color_stops;

            // transparency_stops
            TransparencyStops trns_stops;
            std::vector<TransparencyStopObject> trns_stop_objects;
            for (const auto& alpha : g.alpha_markers) {
                TransparencyStopObject trns_stop_object;
                trns_stop_object.location = static_cast<int32_t>(alpha.position * 4096.0);
                trns_stop_object.midpoint = static_cast<int32_t>(alpha.midpoint * 100.0);
                trns_stop_object.opacity = alpha.value * 100.0;
                trns_stop_objects.push_back(trns_stop_object);
            }
            trns_stops.transparency_stop_objects = trns_stop_objects;
            grd_gradient_object.transparency_stops = trns_stops;

            grd_gradient.gradient_object = grd_gradient_object;
            gradient_list.push_back(grd_gradient);
        }
        grd.gradient_list.gradient_list = gradient_list;

        return grd;
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
        std::vector<ImVec4> color_marker_colors = gradient.m_marker_manager.getMarkerColors();
        auto color_marker_positions             = gradient.m_marker_manager.getMarkerPos();
        auto color_marker_midpoints             = gradient.m_marker_manager.getMidpointRatios();
        for (int32_t i = 0; i < color_marker_num; ++i) {
            uint32_t rgba             = color_conv::vec4normRgba2u32Rgba<ImVec4>(color_marker_colors[i]);
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
            alpha_markers[i].value    = alpha_marker_values[i];
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
    std::vector<std::string> loadCategories(Preset& cfg)
    {
        std::vector<std::string> categories;
        std::unordered_set<std::string> seen;

        if (!cfg.categories.empty()) {
            // 重複無しでカテゴリーを読み込む
            for (const auto& [i, category] : cfg.categories | std::views::enumerate) {
                if (seen.insert(category).second) {
                    categories.push_back(category);
                }
            }
        } else {
            categories.push_back(GradientConfigManager::DEFAULT_CATEGORY);
        }

        return categories;
    }

    ConfigWriteResult addCategory(Preset& cfg, std::string_view name)
    {
        bool category_name_exists = false;
        for (const auto& category : cfg.categories) {
            if (category == name) {
                category_name_exists = true;
                break;
            }
        }

        if (category_name_exists) {
            return {true, ""};
        }

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
