#ifndef GRD_PARSER_H
#define GRD_PARSER_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>


template <std::integral T>
inline std::expected<T, std::string> parseIntBE(std::ifstream& file)
{
    T n = 0;
    if (!file.read(reinterpret_cast<char*>(&n), sizeof(n))) {
        return std::unexpected{std::format("Failed to read {}-byte ASCII from stream", sizeof(T))};
    }
    return std::byteswap(n);
}

inline std::expected<std::string, std::string> parseID(std::ifstream& file)
{
    auto len = parseIntBE<uint32_t>(file);
    if (!len) {
        return std::unexpected{len.error()};
    }
    if (len.value() == 0) {
        len = 4;
    }

    std::string id;
    id.resize(len.value());
    if (!file.read(id.data(), len.value())) {
        return std::unexpected{std::format("Failed to read {}-byte ASCII from stream", len.value())};
    }

    return id;
}

inline std::expected<std::string, std::string> parseItemType(std::ifstream& file)
{
    char item_type[5] = {0};
    if (!file.read(item_type, 4)) {
        return std::unexpected{"Failed to read 4-byte ASCII from stream"};
    }
    return item_type;
}

inline std::expected<int32_t, std::string> parseInt32(std::ifstream& file)
{
    auto item_type = parseItemType(file);
    if (!item_type) {
        return std::unexpected{item_type.error()};
    }
    if (item_type.value() != "long") {
        return std::unexpected{"Failed to parse \"long\""};
    }

    return parseIntBE<int32_t>(file);
}

inline std::expected<int32_t, std::string> parseInt32WithID(std::ifstream& file, std::string_view expected_id)
{
    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != expected_id) {
        return std::unexpected{"Failed to parse " + std::string{expected_id}};
    }

    return parseInt32(file);
}

inline std::expected<std::vector<std::byte>, std::string> parseRawData(std::ifstream& file)
{
    auto item_type = parseItemType(file);
    if (!item_type) {
        return std::unexpected{item_type.error()};
    }
    if (item_type.value() != "tdta") {
        return std::unexpected{"Failed to parse \"tdta\""};
    }

    auto len = parseIntBE<uint32_t>(file);
    if (!len) {
        return std::unexpected{len.error()};
    }

    std::vector<std::byte> raw_data;
    raw_data.resize(len.value());

    if (!file.read(reinterpret_cast<char*>(raw_data.data()), len.value())) {
        return std::unexpected{std::format("Failed to read {}-byte data from stream", len.value())};
    }

    return raw_data;
}

inline std::expected<bool, std::string> parseBoolean(std::ifstream& file)
{
    uint8_t b;
    if (!file.read(reinterpret_cast<char*>(&b), 1)) {
        return std::unexpected{"Failed to read 1-byte from stream"};
    }

    return b != 0;
}

inline std::expected<bool, std::string> parseBool(std::ifstream& file)
{
    auto item_type = parseItemType(file);
    if (!item_type) {
        return std::unexpected{item_type.error()};
    }
    if (item_type.value() != "bool") {
        return std::unexpected{"Failed to parse \"bool\""};
    }

    return parseBoolean(file);
}

inline std::expected<double, std::string> parseDoubleBE(std::ifstream& file)
{
    auto value = parseIntBE<int64_t>(file);
    if (!value) {
        return std::unexpected{value.error()};
    }

    return std::bit_cast<double>(value.value());
}

inline std::expected<double, std::string> parseDouble(std::ifstream& file)
{
    auto item_type = parseItemType(file);
    if (!item_type) {
        return std::unexpected{item_type.error()};
    }
    if (item_type.value() != "doub") {
        return std::unexpected{"Failed to parse doub"};
    }

    return parseDoubleBE(file);
}

inline std::expected<double, std::string> parseDoubleWithID(std::ifstream& file, std::string_view expected_id)
{
    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != expected_id) {
        return std::unexpected{"Failed to parse " + std::string{expected_id}};
    }

    auto item_type = parseItemType(file);
    if (!item_type) {
        return std::unexpected{item_type.error()};
    }
    if (item_type.value() != "doub") {
        return std::unexpected{"Failed to parse doub"};
    }

    return parseDoubleBE(file);
}

inline std::expected<double, std::string> parseUnitDouble(std::ifstream& file, std::string_view expected_unit_id)
{
    auto item_type = parseItemType(file);
    if (!item_type) {
        return std::unexpected{item_type.error()};
    }
    if (item_type.value() != "UntF") {
        return std::unexpected{"Failed to parse \"UntF\""};
    }

    auto unit_id = parseItemType(file);
    if (!unit_id) {
        return std::unexpected{unit_id.error()};
    }
    if (unit_id.value() != expected_unit_id) {
        return std::unexpected{"Failed to parse " + std::string{expected_unit_id}};
    }

    return parseDoubleBE(file);
}

inline std::expected<double, std::string> parseUnitDoubleWithID(std::ifstream& file, std::string_view expected_unit_id, std::string_view expected_id)
{
    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != expected_id) {
        return std::unexpected{"Failed to parse " + std::string{expected_id}};
    }

    return parseUnitDouble(file, expected_unit_id);
}

inline std::expected<std::string, std::string> parseUTF16BE(std::ifstream& file, const uint32_t str_len)
{
    if (str_len == 0) {
        return std::unexpected{"文字数の長さが不正です"};
    }

    uint32_t byte_len = str_len * 2;
    std::vector<uint8_t> buffer(byte_len);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), byte_len)) {
        return std::unexpected{"ファイルからの読み込みに失敗しました"};
    }

    std::u16string u16str;
    u16str.reserve(str_len);
    for (size_t i = 0; i < byte_len - 1; i += 2) {
        char16_t c = (static_cast<char16_t>(buffer[i]) << 8) | (static_cast<char16_t>(buffer[i + 1]));
        if (c == 0) break;
        u16str.push_back(c);
    }

    int32_t size = ::WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<LPCWCH>(u16str.data()), u16str.size(), nullptr, 0, nullptr, nullptr);
    std::string u8str(size, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, reinterpret_cast<LPCWCH>(u16str.data()), u16str.size(), u8str.data(), size, nullptr, nullptr);

    return u8str;
}

inline std::expected<int32_t, std::string> parseListItemNum(std::ifstream& file)
{
    auto item_type = parseItemType(file);
    if (!item_type) {
        return std::unexpected{item_type.error()};
    }
    if (item_type != "VlLs") {
        return std::unexpected{"Failed to parse \"VlLs\""};
    }

    auto list_item_num = parseIntBE<int32_t>(file);
    if (!list_item_num) {
        return std::unexpected{list_item_num.error()};
    }

    return list_item_num.value();
}

struct Class {
    std::string name;
    std::string id;
};

inline std::expected<std::string, std::string> parseUnicodeString(std::ifstream& file)
{
    auto item_type = parseItemType(file);
    if (!item_type) {
        return std::unexpected{item_type.error()};
    }
    if (item_type != "TEXT") {
        return std::unexpected{"Failed to parse \"TEXT\""};
    }

    auto text_len = parseIntBE<uint32_t>(file);
    if (!text_len) {
        return std::unexpected{text_len.error()};
    }

    auto text = parseUTF16BE(file, text_len.value());
    if (!text) {
        return std::unexpected{text.error()};
    }

    return text.value();
}

struct Object {
    Class class_;
    uint32_t key_item_num;
};

inline std::expected<Object, std::string> parseObject(std::ifstream& file)
{
    auto item_type = parseItemType(file);
    if (!item_type) {
        return std::unexpected{item_type.error()};
    }
    if (item_type.value() != "Objc") {
        return std::unexpected{"Failed to parse \"Objc\""};
    }

    auto class_name_len = parseIntBE<uint32_t>(file);
    if (!class_name_len) {
        return std::unexpected{class_name_len.error()};
    }

    auto class_name = parseUTF16BE(file, class_name_len.value());
    if (!class_name) {
        return std::unexpected{class_name.error()};
    }

    auto class_id = parseID(file);
    if (!class_id) {
        return std::unexpected{class_id.error()};
    }

    auto key_item_num = parseIntBE<uint32_t>(file);
    if (!key_item_num) {
        return std::unexpected{key_item_num.error()};
    }

    return Object{Class{class_name.value(), class_id.value()}, key_item_num.value()};
}

inline std::expected<uint64_t, std::string> parseInterpolation(std::ifstream& file)
{
    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "Intr") {
        return std::unexpected{"Failed to parse \"Intr\""};
    }

    return parseDouble(file);
}

inline std::expected<std::string, std::string> parseName(std::ifstream& file)
{
    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "Nm  ") {
        return std::unexpected{"Failed to parse \"Nm  \""};
    }

    return parseUnicodeString(file);
}

struct BookColor {
    std::string book_name{};
    std::string color_name{};
    int32_t book_id{};
    std::vector<std::byte> book_key{};
};

inline std::expected<std::string, std::string> parseBk(std::ifstream& file)
{
    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "Bk  ") {
        return std::unexpected{"Failed to parse \"Bk  \""};
    }

    return parseUnicodeString(file);
}

inline std::expected<int32_t, std::string> parseBookID(std::ifstream& file)
{
    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "bookID") {
        return std::unexpected{"Failed to parse \"bookID\""};
    }

    return parseInt32(file);
}

inline std::expected<std::vector<std::byte>, std::string> parseBookKey(std::ifstream& file)
{
    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "bookKey") {
        return std::unexpected{"Failed to parse \"bookKey\""};
    }

    return parseRawData(file);
}

inline std::expected<BookColor, std::string> parseBkCl(std::ifstream& file)
{
    auto book_name = parseBk(file);
    if (!book_name) {
        return std::unexpected{book_name.error()};
    }

    auto color_name = parseName(file);
    if (!color_name) {
        return std::unexpected{color_name.error()};
    }

    auto book_id = parseBookID(file);
    if (!book_id) {
        return std::unexpected{book_id.error()};
    }

    auto book_key = parseBookKey(file);
    if (!book_key) {
        return std::unexpected{book_key.error()};
    }

    return BookColor{book_name.value(), color_name.value(), book_id.value(), book_key.value()};
}

struct Enumerated {
    std::string type_id;
    std::string value_id;
};

inline std::expected<Enumerated, std::string> parseEnumrated(std::ifstream& file)
{
    auto item_type = parseItemType(file);
    if (!item_type) {
        return std::unexpected{item_type.error()};
    }
    if (item_type != "enum") {
        return std::unexpected{"Failed to parse \"enum\""};
    }

    auto enum_type_id = parseID(file);
    if (!enum_type_id) {
        return std::unexpected{enum_type_id.error()};
    }

    auto enum_value_id = parseID(file);
    if (!enum_value_id) {
        return std::unexpected{enum_value_id.error()};
    }
    return Enumerated{enum_type_id.value(), enum_value_id.value()};
}

inline std::expected<std::string, std::string> parseEnumratedWithTypeID(std::ifstream& file, std::string_view expected_type_id)
{
    auto item_type = parseItemType(file);
    if (!item_type) {
        return std::unexpected{item_type.error()};
    }
    if (item_type != "enum") {
        return std::unexpected{"Failed to parse \"enum\""};
    }

    auto enum_type_id = parseID(file);
    if (!enum_type_id) {
        return std::unexpected{enum_type_id.error()};
    }
    if (enum_type_id.value() != expected_type_id) {
        return std::unexpected{std::format("Failed to parse \"{}\"", expected_type_id)};
    }

    auto enum_value_id = parseID(file);
    if (!enum_value_id) {
        return std::unexpected{enum_value_id.error()};
    }

    return enum_value_id.value();
}

inline std::expected<Enumerated, std::string> parseGradientForm(std::ifstream& file)
{
    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "GrdF") {
        return std::unexpected{"Failed to parse \"GrdF\""};
    }

    return parseEnumrated(file);
}

inline std::expected<Enumerated, std::string> parseType(std::ifstream& file)
{
    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "Type") {
        return std::unexpected{"Failed to parse \"Type\""};
    }

    return parseEnumrated(file);
}

struct HSBC {
    double hue;
    double saturate;
    double brightness;
};

inline std::expected<HSBC, std::string> parseHSBC(std::ifstream& file)
{
    auto hue = parseUnitDoubleWithID(file, "#Ang", "H   ");
    if (!hue) {
        return std::unexpected{hue.error()};
    }

    auto saturate = parseDoubleWithID(file, "Strt");
    if (!saturate) {
        return std::unexpected{saturate.error()};
    }

    auto brightness = parseDoubleWithID(file, "Brgh");
    if (!brightness) {
        return std::unexpected{brightness.error()};
    }

    return HSBC{hue.value(), saturate.value(), brightness.value()};
}

struct RGBC {
    double red;
    double green;
    double blue;
};

inline std::expected<RGBC, std::string> parseRGBC(std::ifstream& file)
{
    auto red = parseDoubleWithID(file, "Rd  ");
    if (!red) {
        return std::unexpected{red.error()};
    }

    auto green = parseDoubleWithID(file, "Grn ");
    if (!green) {
        return std::unexpected{green.error()};
    }

    auto blue = parseDoubleWithID(file, "Bl  ");
    if (!blue) {
        return std::unexpected{blue.error()};
    }

    return RGBC{red.value(), green.value(), blue.value()};
}

struct CMYC {
    double cyan;
    double magenta;
    double yellow;
    double black;
};

inline std::expected<CMYC, std::string> parseCMYC(std::ifstream& file)
{
    auto cyan = parseDoubleWithID(file, "Cyn ");
    if (!cyan) {
        return std::unexpected{cyan.error()};
    }

    auto magenta = parseDoubleWithID(file, "Mgnt");
    if (!magenta) {
        return std::unexpected{magenta.error()};
    }

    auto yellow = parseDoubleWithID(file, "Ylw ");
    if (!yellow) {
        return std::unexpected{yellow.error()};
    }

    auto black = parseDoubleWithID(file, "Blck");
    if (!black) {
        return std::unexpected{black.error()};
    }

    return CMYC{magenta.value(), magenta.value(), yellow.value(), black.value()};
}

struct Grsc {
    double gray;
};

inline std::expected<Grsc, std::string> parseGrsc(std::ifstream& file)
{
    auto gray = parseDoubleWithID(file, "Gry ");
    if (!gray) {
        return std::unexpected{gray.error()};
    }

    return Grsc{gray.value()};
}

struct LbCl {
    double luminance;
    double a;
    double b;
};

inline std::expected<LbCl, std::string> parseLbCl(std::ifstream& file)
{
    auto luminance = parseDoubleWithID(file, "Lmnc");
    if (!luminance) {
        return std::unexpected{luminance.error()};
    }

    auto a = parseDoubleWithID(file, "A   ");
    if (!a) {
        return std::unexpected{a.error()};
    }

    auto b = parseDoubleWithID(file, "B   ");
    if (!b) {
        return std::unexpected{b.error()};
    }

    return LbCl{luminance.value(), a.value(), b.value()};
}

using ColorObject = std::variant<BookColor, CMYC, Grsc, HSBC, LbCl, RGBC>;

struct PrintColorObject {
    void operator()(BookColor color) const
    {
        std::cout << std::format("book_name: {}", color.book_name) << "\n";
        std::cout << std::format("color_name: {}", color.color_name) << "\n";
        std::cout << std::format("book_id: {}", color.book_id) << "\n";
        std::string book_key_str(reinterpret_cast<const char*>(color.book_key.data()), color.book_key.size());
        std::cout << std::format("book_key: {}", book_key_str) << "\n";
    }

    void operator()(CMYC color) const
    {
        std::cout << std::format("cyan: {}", color.cyan) << "\n";
        std::cout << std::format("magenta: {}", color.magenta) << "\n";
        std::cout << std::format("yellow: {}", color.yellow) << "\n";
        std::cout << std::format("black: {}", color.black) << "\n";
    }

    void operator()(Grsc color) const
    {
        std::cout << std::format("gray: {}", color.gray) << "\n";
    }

    void operator()(HSBC color) const
    {
        std::cout << std::format("hue: {}", color.hue) << "\n";
        std::cout << std::format("saturate: {}", color.saturate) << "\n";
        std::cout << std::format("brightness: {}", color.brightness) << "\n";
    }

    void operator()(LbCl color) const
    {
        std::cout << std::format("luminance: {}", color.luminance) << "\n";
        std::cout << std::format("a: {}", color.a) << "\n";
        std::cout << std::format("b: {}", color.b) << "\n";
    }

    void operator()(RGBC color) const
    {
        std::cout << std::format("red: {}", color.red) << "\n";
        std::cout << std::format("green: {}", color.green) << "\n";
        std::cout << std::format("blue: {}", color.blue) << "\n";
    }
};

struct ColorStopObject {
    double location;
    double midpoint;
    std::string type;
    ColorObject color_object;

    void print() const
    {
        std::cout << std::format("location: {}", location) << "\n";
        std::cout << std::format("midpoint: {}", midpoint) << "\n";
        std::cout << std::format("type: {}", type) << "\n";
        std::visit(PrintColorObject{}, color_object);
    }
};

struct TrancparencyStopObject {
    double opacity{};
    double location{};
    double midpoint{};

    void print() const
    {
        std::cout << std::format("opacity: {}", opacity) << "\n";
        std::cout << std::format("location: {}", location) << "\n";
        std::cout << std::format("midpoint: {}", midpoint) << "\n";
    }
};

inline std::expected<ColorObject, std::string> parseColorObject(std::ifstream& file)
{
    auto objc = parseObject(file);
    if (!objc) {
        return std::unexpected{objc.error()};
    }

    std::string color_type = objc.value().class_.id;
    if (color_type == "BkCl") {
        auto book_color = parseBkCl(file);
        if (!book_color) {
            return std::unexpected{book_color.error()};
        }
        return book_color.value();
    } else if (color_type == "CMYC") {
        auto cmyk = parseCMYC(file);
        if (!cmyk) {
            return std::unexpected{cmyk.error()};
        }
        return cmyk.value();
    } else if (color_type == "Grsc") {
        auto gray = parseGrsc(file);
        if (!gray) {
            return std::unexpected{gray.error()};
        }
        return gray.value();
    } else if (color_type == "HSBC") {
        auto hsb = parseHSBC(file);
        if (!hsb) {
            return std::unexpected{hsb.error()};
        }
        return hsb.value();
    } else if (color_type == "LbCl") {
        auto lab = parseLbCl(file);
        if (!lab) {
            return std::unexpected{lab.error()};
        }
        return lab.value();
    } else if (color_type == "RGBC") {
        auto rgb = parseRGBC(file);
        if (!rgb) {
            return std::unexpected{rgb.error()};
        }
        return rgb.value();
    }

    return std::unexpected{"Failed to parse Color Object"};
}

inline std::expected<Enumerated, std::string> parseColorStopType(std::ifstream& file)
{
    auto type = parseType(file);
    if (!type) {
        return std::unexpected{type.error()};
    }
    if (type.value().type_id != "Clry") {
        return std::unexpected{"Failed to parse \"Clry\""};
    }

    return type;
}

inline std::expected<ColorStopObject, std::string> parseColorStopObject(std::ifstream& file)
{
    ColorStopObject color_stop_object;

    auto objc = parseObject(file);
    if (!objc) {
        return std::unexpected{objc.error()};
    }
    if (objc.value().class_.id != "Clrt") {
        return std::unexpected{"Failed to parse \"Clrt\""};
    }

    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "Clr ") {
        return std::unexpected{"Failed to parse \"Clr \""};
    }

    auto color_object = parseColorObject(file);
    if (!color_object) {
        return std::unexpected{color_object.error()};
    }
    color_stop_object.color_object = color_object.value();

    auto color_stop_type = parseColorStopType(file);
    if (!color_stop_type) {
        return std::unexpected{color_stop_type.error()};
    }
    color_stop_object.type = color_stop_type.value().value_id;

    auto location = parseInt32WithID(file, "Lctn");
    if (!location) {
        return std::unexpected{"Failed to parse \"Lctn\""};
    }
    color_stop_object.location = location.value();

    auto midpoint = parseInt32WithID(file, "Mdpn");
    if (!midpoint) {
        return std::unexpected{"Failed to parse \"Mdpn\""};
    }
    color_stop_object.midpoint = midpoint.value();

    return color_stop_object;
}

struct ColorStops {
    int32_t item_num{};
    std::vector<ColorStopObject> color_stop_objects{};

    void print() const
    {
        std::cout << std::format("item_num: {}", item_num) << "\n";
        for (const auto& e : color_stop_objects) {
            e.print();
        }
    }
};

inline std::expected<ColorStops, std::string> parseColorStops(std::ifstream& file)
{
    ColorStops color_stops;

    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "Clrs") {
        return std::unexpected{"Failed to parse \"Clrs\""};
    }

    auto list_item_num = parseListItemNum(file);
    if (!list_item_num) {
        return std::unexpected{list_item_num.error()};
    }
    color_stops.item_num = list_item_num.value();

    std::vector<ColorStopObject> color_stop_objects(color_stops.item_num);
    for (auto& object : color_stop_objects) {
        auto color_stop_object = parseColorStopObject(file);
        if (!color_stop_object) {
            return std::unexpected{color_stop_object.error()};
        }
        object = color_stop_object.value();
    }
    color_stops.color_stop_objects = color_stop_objects;

    return color_stops;
}

struct TransparencyStopObject {
    int32_t location{};
    int32_t midpoint{};
    double opacity{};

    void print() const
    {
        std::cout << std::format("location: {}", location) << "\n";
        std::cout << std::format("midpoint: {}", midpoint) << "\n";
        std::cout << std::format("opacity: {}", opacity) << "\n";
    }
};

inline std::expected<TransparencyStopObject, std::string> parseTransparencyStopObject(std::ifstream& file)
{
    TransparencyStopObject transparency_stop_object;

    auto objc = parseObject(file);
    if (!objc) {
        return std::unexpected{objc.error()};
    }
    if (objc.value().class_.id != "TrnS") {
        return std::unexpected{"Failed to parse \"TrnS\""};
    }

    auto opacity = parseUnitDoubleWithID(file, "#Prc", "Opct");
    if (!opacity) {
        return std::unexpected{opacity.error()};
    }
    transparency_stop_object.opacity = opacity.value();

    auto location = parseInt32WithID(file, "Lctn");
    if (!location) {
        return std::unexpected{location.error()};
    }
    transparency_stop_object.location = location.value();

    auto midpoint = parseInt32WithID(file, "Mdpn");
    if (!midpoint) {
        return std::unexpected{midpoint.error()};
    }
    transparency_stop_object.midpoint = midpoint.value();

    return transparency_stop_object;
}

struct TransparencyStops {
    std::vector<TransparencyStopObject> transparency_stop_objects;

    void print() const
    {
        for (const auto& e : transparency_stop_objects) {
            e.print();
        }
    }
};

inline std::expected<TransparencyStops, std::string> parseTransparencyStops(std::ifstream& file)
{
    TransparencyStops transparency_stops;

    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "Trns") {
        return std::unexpected{"Failed to parse \"Trns\""};
    }

    auto list_item_num = parseListItemNum(file);
    if (!list_item_num) {
        return std::unexpected{list_item_num.error()};
    }

    std::vector<TransparencyStopObject> stop_objects(list_item_num.value());
    for (auto& e : stop_objects) {
        auto stop_object = parseTransparencyStopObject(file);
        if (!stop_object) {
            return std::unexpected{stop_object.error()};
        }
        e = stop_object.value();
    }
    transparency_stops.transparency_stop_objects = stop_objects;

    return transparency_stops;
}

struct CustomStopsGradientObject {
    double interpolation{};
    ColorStops color_stops{};
    TransparencyStops transparency_stops{};

    void print() const
    {
        std::cout << std::format("interpolation: {}", interpolation) << "\n";
        color_stops.print();
        transparency_stops.print();
    }
};

inline std::expected<CustomStopsGradientObject, std::string> parseCustomStopsGradientObject(std::ifstream& file)
{
    CustomStopsGradientObject custom_sops_gradient_object;

    auto interpolation = parseInterpolation(file);
    if (!interpolation) {
        return std::unexpected{interpolation.error()};
    }
    custom_sops_gradient_object.interpolation = interpolation.value();

    auto color_stops = parseColorStops(file);
    if (!color_stops) {
        return std::unexpected{color_stops.error()};
    }
    custom_sops_gradient_object.color_stops = color_stops.value();

    auto transparency_stops = parseTransparencyStops(file);
    if (!transparency_stops) {
        return std::unexpected{transparency_stops.error()};
    }
    custom_sops_gradient_object.transparency_stops = transparency_stops.value();

    return custom_sops_gradient_object;
}

struct ColorNoiseGradientObject {
    bool show_transparency{};
    bool vector_color{};
    std::string color_space;
    int32_t random_seed;
    int32_t smoothness;
    std::array<double, 4> minimum_values{};
    std::array<double, 4> maximum_values{};

    void print() const
    {
        std::cout << std::format("show_transparency: {}", show_transparency) << "\n";
        std::cout << std::format("vector_color: {}", vector_color) << "\n";
        std::cout << std::format("color_space: {}", color_space) << "\n";
        std::cout << std::format("random_seed: {}", random_seed) << "\n";
        std::cout << std::format("smoothness: {}", smoothness) << "\n";
        std::cout << std::format("minimum_values = [{}, {}, {}, {}]", minimum_values[0], minimum_values[1], minimum_values[2], minimum_values[3]) << "\n";
        std::cout << std::format("maximum_values = [{}, {}, {}, {}]", maximum_values[0], maximum_values[1], maximum_values[2], maximum_values[3]) << "\n";
    }
};

inline std::expected<ColorNoiseGradientObject, std::string> parseColorNoiseGradientObject(std::ifstream& file)
{
    ColorNoiseGradientObject color_noise_gradient_object;

    auto id = parseID(file);
    if (id != "ShTr") {
        return std::unexpected{id.error()};
    }
    auto show_transparency = parseBool(file);
    if (!show_transparency) {
        return std::unexpected{show_transparency.error()};
    }
    color_noise_gradient_object.show_transparency = show_transparency.value();

    id = parseID(file);
    if (id != "VctC") {
        return std::unexpected{id.error()};
    }
    auto vector_color = parseBool(file);
    if (!vector_color) {
        return std::unexpected{vector_color.error()};
    }
    color_noise_gradient_object.vector_color = vector_color.value();

    id = parseID(file);
    if (id != "ClrS") {
        return std::unexpected{id.error()};
    }
    auto color_space = parseEnumratedWithTypeID(file, "ClrS");
    if (!color_space) {
        return std::unexpected{color_space.error()};
    }
    color_noise_gradient_object.color_space = color_space.value();

    auto random_seed = parseInt32WithID(file, "RndS");
    if (!random_seed) {
        return std::unexpected{random_seed.error()};
    }
    color_noise_gradient_object.random_seed = random_seed.value();

    auto smoothness = parseInt32WithID(file, "Smth");
    if (!smoothness) {
        return std::unexpected{smoothness.error()};
    }
    color_noise_gradient_object.smoothness = smoothness.value();

    id = parseID(file);
    if (id != "Mnm ") {
        return std::unexpected{id.error()};
    }
    [[maybe_unused]] auto list_num = parseListItemNum(file);
    for (auto& e : color_noise_gradient_object.minimum_values) {
        auto value = parseDouble(file);
        if (!value) {
            return std::unexpected{value.error()};
        }
        e = value.value();
    }

    id = parseID(file);
    if (id != "Mxm ") {
        return std::unexpected{id.error()};
    }
    list_num = parseListItemNum(file);
    for (auto& e : color_noise_gradient_object.maximum_values) {
        auto value = parseDouble(file);
        if (!value) {
            return std::unexpected{value.error()};
        }
        e = value.value();
    }

    return color_noise_gradient_object;
}

using GradientObject = std::variant<CustomStopsGradientObject, ColorNoiseGradientObject>;

struct PrintGradientObject {
    void operator()(CustomStopsGradientObject gradient_object) const
    {
        gradient_object.print();
    }

    void operator()(ColorNoiseGradientObject gradient_object) const
    {
        gradient_object.print();
    }
};

struct Gradient {
    std::string gradient_name;
    std::string gradient_form;
    GradientObject gradient_object;

    void print() const
    {
        std::cout << std::format("gradient_name: {}", gradient_name) << "\n";
        std::cout << std::format("gradient_form: {}", gradient_form) << "\n";
        std::visit(PrintGradientObject{}, gradient_object);
    }
};

inline std::expected<Gradient, std::string> parseGradient(std::ifstream& file)
{
    Gradient gradient;

    auto object = parseObject(file);
    if (!object) {
        return std::unexpected{object.error()};
    }

    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "Grad") {
        return std::unexpected{"Failed to parse \"Grad\""};
    }
    object = parseObject(file);
    if (!object) {
        return std::unexpected{object.error()};
    }

    auto gradient_name = parseName(file);
    if (!gradient_name) {
        return std::unexpected{gradient_name.error()};
    }
    gradient.gradient_name = gradient_name.value();

    auto gradient_form = parseGradientForm(file);
    if (!gradient_form) {
        return std::unexpected{gradient_form.error()};
    }
    gradient.gradient_form = gradient_form.value().value_id;

    if (gradient_form.value().value_id == "CstS") {
        auto custom_sops_gradient_object = parseCustomStopsGradientObject(file);
        if (!custom_sops_gradient_object) {
            return std::unexpected{custom_sops_gradient_object.error()};
        }
        gradient.gradient_object = custom_sops_gradient_object.value();
    } else if (gradient_form.value().value_id == "ClNs") {
        auto color_noise_gradient_object = parseColorNoiseGradientObject(file);
        if (!color_noise_gradient_object) {
            return std::unexpected{color_noise_gradient_object.error()};
        }
        gradient.gradient_object = color_noise_gradient_object.value();
    }

    return gradient;
}

struct GradientList {
    std::vector<Gradient> gradient_list;

    void print() const
    {
        for (const auto& e : gradient_list) {
            e.print();
        }
    }
};

inline std::expected<GradientList, std::string> parseGradientList(std::ifstream& file)
{
    auto id = parseID(file);
    if (!id) {
        return std::unexpected{id.error()};
    }
    if (id.value() != "GrdL") {
        return std::unexpected{"Failed to parse \"GrdL\""};
    }

    auto list_item_num = parseListItemNum(file);
    if (!list_item_num) {
        return std::unexpected{list_item_num.error()};
    }

    std::vector<Gradient> gradient_list(list_item_num.value());

    for (auto& e : gradient_list) {
        auto gradient = parseGradient(file);
        if (!gradient) {
            return std::unexpected{gradient.error()};
        }
        e = gradient.value();
    }

    return GradientList{gradient_list};
}

struct Header {
    std::string signature{};
    uint16_t version{};
    uint32_t descriptor_version{};
};

inline std::expected<Header, std::string> parseHeader(std::ifstream& file)
{
    Header header;

    char signature_[5] = {0};
    if (!file.read(signature_, 4)) {
        return std::unexpected{"Failed to parse signature"};
    }
    std::string signature = signature_;
    header.signature      = signature;

    auto version = parseIntBE<uint16_t>(file);
    if (!version) {
        return std::unexpected{version.error()};
    }
    header.version = version.value();

    auto descriptor_version = parseIntBE<uint32_t>(file);
    if (!descriptor_version) {
        return std::unexpected{descriptor_version.error()};
    }
    header.descriptor_version = descriptor_version.value();

    return header;
}

struct GRD {
    Header header;
    GradientList gradient_list;
};

inline std::expected<GRD, std::string> parseGRD(const std::filesystem::path& file_path)
{
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        return std::unexpected{"Failed to load the file"};
    }

    auto header = parseHeader(file);
    if (!header) {
        return std::unexpected{header.error()};
    }
    if (header.value().signature != "8BGR") {
        return std::unexpected{"Unsupported file format"};
    }
    if (header.value().version != 5) {
        return std::unexpected{"Unsupported version"};
    }

    // 未知領域
    file.ignore(18);

    auto gradient_list = parseGradientList(file);
    if (!gradient_list) {
        return std::unexpected{gradient_list.error()};
    }

    // フッターは不要のためパースしない

    return GRD{header.value(), gradient_list.value()};
}

#endif
