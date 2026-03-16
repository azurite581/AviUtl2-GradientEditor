#ifndef ALIAS_PARSER_H
#define ALIAS_PARSER_H

#include <algorithm>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

namespace alias_parser {
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
