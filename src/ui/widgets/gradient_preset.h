#ifndef GRADIENT_PRESET_H
#define GRADIENT_PRESET_H

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "json.hpp"

namespace preset {
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
}  // namespace preset

struct GradientConfig {
    std::vector<std::string> categories{"uncategorized"};
    std::vector<preset::GradientPreset> presets;
};

inline void to_json(nlohmann::ordered_json& j, const GradientConfig& config)
{
    j = nlohmann::ordered_json{
        {"categories", config.categories},
        {"presets", config.presets}
    };
}

inline void from_json(const nlohmann::ordered_json& j, GradientConfig& config)
{
    config.categories = j.value("categories", config.categories);
    config.presets = j.value("presets", config.presets);
}

class PresetManager
    : public preset::GradientPreset,
      public GradientConfig {
private:
    std::filesystem::path m_preset_path;
    const GradientConfig DEFAULT_PRESET_FILE{GradientConfig{ .categories = {"uncategorized"}, .presets = {preset::GradientPreset{}}}};
    inline static const char* DEFAULT_PRESET_FILE_JSON = R"(
{
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
    ]
}
)";

public:
    PresetManager() = default;

    PresetManager(const std::filesystem::path& path)
        : m_preset_path{path}
    {
    }

    void setPresetFilePath(const std::filesystem::path& path) { m_preset_path = path; }

    struct PresetLoadResult {
        GradientConfig preset_file;
        std::string error;  // 空なら成功
    };
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
    PresetWriteResult writePresetFile(const GradientConfig& preset_file)
    {
        PresetWriteResult result{false, {}};

        nlohmann::ordered_json j = preset_file;
        std::ofstream ofs(m_preset_path);
        if (!ofs) {
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

    static bool containsCategory(const std::vector<std::string>& categories, std::string_view target_category)
    {
        for (const auto& c : categories) {
            if (c == target_category) return true;
        }
        return false;
    }

};

#endif  // !GRADIENT_PRESET_H
