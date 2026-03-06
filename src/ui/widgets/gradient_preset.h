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
class GradientPreset {
public:
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
    j.at("name").get_to(preset.name);
    j.at("colors").get_to(preset.colors);
    j.at("positions").get_to(preset.positions);
    j.at("midpoints").get_to(preset.midpoints);
    j.at("blur_width").get_to(preset.blur_width);
    j.at("color_space").get_to(preset.color_space);
    j.at("interpolation_path").get_to(preset.interpolation_path);
}
}  // namespace preset

namespace preset_file {
struct GradientPresetFile {
    std::vector<preset::GradientPreset> presets;
};

inline void to_json(nlohmann::ordered_json& j, const GradientPresetFile& preset_file)
{
    j = nlohmann::ordered_json{{"presets", preset_file.presets}};
}

inline void from_json(const nlohmann::ordered_json& j, GradientPresetFile& preset_file)
{
    j.at("presets").get_to(preset_file.presets);
}

}  // namespace preset_file

class PresetManager
    : public preset::GradientPreset,
      public preset_file::GradientPresetFile {
private:
    std::filesystem::path m_preset_path;
    const preset_file::GradientPresetFile DEFAULT_PRESET_FILE{preset_file::GradientPresetFile{.presets = {preset::GradientPreset{}}}};
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
        preset_file::GradientPresetFile preset_file;
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
            result.preset_file = j.get<preset_file::GradientPresetFile>();
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
    PresetWriteResult writePresetFile(const preset_file::GradientPresetFile& preset_file)
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
};

#endif  // !GRADIENT_PRESET_H
