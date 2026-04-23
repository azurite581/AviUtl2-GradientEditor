#ifndef ALIAS_PARSER_H
#define ALIAS_PARSER_H

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <expected>
#include <ranges>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

namespace alias_parser {

template <typename T>
std::expected<T, std::string> parseValue(std::string_view sv);

template <std::integral T>
inline std::expected<T, std::string> parseValue(std::string_view s)
{
    int32_t base = 10;
    if (s.empty() || base < 2 || base > 36) return std::unexpected("Invalid input");

    if (s.starts_with("0x") || s.starts_with("0X")) {
        base = 16;
        s.remove_prefix(2);
    } else if (s.starts_with("0b") || s.starts_with("0B")) {
        base = 2;
        s.remove_prefix(2);
    }

    T value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value, base);
    if (ec == std::errc()) return value;
    return std::unexpected("Failed to parse value");
}

template <>
inline std::expected<float, std::string> parseValue<float>(std::string_view sv) {
    try {
        size_t idx;
        float v = std::stof(std::string(sv), &idx);
        if (idx != sv.size()) return std::unexpected("Invalid value");
        return v;
    } catch (...) {
        return std::unexpected("Failed to parse value");
    }
}

template <>
inline std::expected<double, std::string> parseValue<double>(std::string_view sv) {
    try {
        size_t idx;
        double v = std::stod(std::string(sv), &idx);
        if (idx != sv.size()) return std::unexpected("Invalid value");
        return v;
    } catch (...) {
        return std::unexpected("Failed to parse value");
    }
}

template <>
inline std::expected<std::string, std::string> parseValue<std::string>(std::string_view sv) {
    return std::string(sv);
}

inline std::string_view trim(std::string_view sv) {
    auto is_space = [](unsigned char c) { return std::isspace(c); };

    while (!sv.empty() && is_space(sv.front())) sv.remove_prefix(1);
    while (!sv.empty() && is_space(sv.back()))  sv.remove_suffix(1);

    return sv;
}

template <typename T>
inline std::expected<std::vector<T>, std::string> splitStr(std::string_view s, std::string_view delim) {
    std::vector<T> result;

    for (auto sv : s | std::views::split(delim)) {
        auto part = trim(std::string_view{sv.begin(), sv.end()});

        auto v = parseValue<T>(part);
        if (!v) return std::unexpected(v.error());

        result.push_back(*v);
    }

    return result;
}

/// @brief エイリアスから指定したセクションの値を文字列で取得する
/// @param src エイリアス文字列 1 行分
/// @param index 取得したい値があるセクションのインデックス
/// @return 指定したセクションの値の文字列
inline std::string getNthToken(std::string_view src, const uint32_t index)
{
    uint32_t start   = 0;
    uint32_t current = 0;

    while (true) {
        auto pos = src.find(',', start);
        if (current == index) {
            return std::string(
                pos == std::string_view::npos
                    ? src.substr(start)
                    : src.substr(start, pos - start));
        }
        if (pos == std::string_view::npos) break;
        start = static_cast<uint32_t>(pos) + 1u;
        ++current;
    }

    auto first = src.find(',');
    return std::string(first == std::string_view::npos ? src : src.substr(0, first));
}

/// @brief 指定したセクションの値を置換する
/// @param src エイリアス文字列 1 行分
/// @param index 置換したい値のあるセクションのインデックス
/// @param replacement 置換に使う値の文字列
/// @return 置換後のエイリアス文字列 1 行分
inline std::string replaceNthToken(std::string_view src, const uint32_t index, std::string_view replacement)
{
    uint32_t start   = 0;
    uint32_t current = 0;
    std::string result{};

    auto slice = [](const std::string& s, const int32_t first, const int32_t end) {
        return s.substr(first, std::abs(end - first) + 1);
    };

    while (true) {
        auto pos = src.find(',', start);
        if (current == index) {
            // 最後の要素ならカンマを付けない
            if (pos == std::string_view::npos) {
                result += std::string{replacement};
                break;
            }
            result += std::string{replacement} + ",";
            start = static_cast<uint32_t>(pos) + 1;
            ++current;
            continue;
        }

        if (pos == std::string_view::npos) {
            if (index >= current + 1) {
                result = std::string{replacement};
            } else {
                result += slice(std::string{src}, start, static_cast<int32_t>(std::ssize(src)) - 1);
            }
            break;
        }

        result += slice(std::string{src}, start, static_cast<int32_t>(pos));
        start = static_cast<uint32_t>(pos) + 1;
        ++current;
    }

    return result;
}

/// @brief エイリアスの [Object] セクションにある frame=<frame1>,<frame2>,... の <frame> の数をカウントする
/// @param alias エイリアス文字列全体
/// @return フレーム数
inline int32_t getFrameCount(std::string_view alias)
{
    int32_t count{2};  // デフォルトは開始と終了の 2
    // [Object] セクションにある frame= 行と一致する
    std::regex re(R"(\[Object\][\s\S]*?\r?\n(frame=.*))");
    std::string s{alias};
    std::smatch m;
    if (std::regex_search(s, m, re)) {
        std::string frame_line = m[1].str();
        count                  = static_cast<int32_t>(std::count(frame_line.begin(), frame_line.end(), ',')) + 1;
    }
    return count;
}
}  // namespace alias_parser

#endif  // !ALIAS_PARSER_H
