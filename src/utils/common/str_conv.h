#ifndef STR_CONV_H
#define STR_CONV_H

#include <charconv>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace str_conv {

/// @brief マルチバイト文字(UTF-8等)をワイド文字(UTF-16)に変換する
/// @param str マルチバイト文字列
/// @param code_page コードページ (デフォルト: CP_UTF8)
/// @return 変換されたワイド文字列
inline std::wstring multiByteToWideChar(std::string_view str, uint32_t code_page = CP_UTF8)
{
    if (str.empty()) {
        return {};
    }

    // 必要なバッファサイズを取得
    int size_needed = ::MultiByteToWideChar(
        code_page,
        0,
        str.data(),
        static_cast<int>(str.size()),
        nullptr,
        0);

    if (size_needed <= 0) {
        return {};
    }

    std::wstring result(size_needed, L'\0');

    ::MultiByteToWideChar(
        code_page,
        0,
        str.data(),
        static_cast<int>(str.size()),
        result.data(),
        size_needed);

    return result;
}

/// @brief ワイド文字(UTF-16)をマルチバイト文字(UTF-8等)に変換する
/// @param str ワイド文字列
/// @param code_page コードページ (デフォルト: CP_UTF8)
/// @return 変換されたマルチバイト文字列
inline std::string wideCharToMultiByte(std::wstring_view str, uint32_t code_page = CP_UTF8)
{
    if (str.empty()) {
        return {};
    }

    DWORD flags = 0;
    if (code_page == CP_UTF8) {
        flags = WC_ERR_INVALID_CHARS;
    }

    // 必要なバッファサイズを取得
    int size_needed = ::WideCharToMultiByte(
        code_page,
        flags,
        str.data(),
        static_cast<int>(str.size()),
        nullptr,
        0,
        nullptr,
        nullptr);

    if (size_needed <= 0) {
        return {};
    }

    std::string result(size_needed, '\0');

    ::WideCharToMultiByte(
        code_page,
        flags,
        str.data(),
        static_cast<int>(str.size()),
        result.data(),
        size_needed,
        nullptr,
        nullptr);

    return result;
}

/// @brief 文字列を整数型に変換する
/// @tparam T 整数型
/// @param s 変換する文字列
/// @param def 変換に失敗した際に返すデフォルト値
/// @param base 基数 [2, 36] (デフォルト: 10)
/// @return 変換された整数値
template <std::integral T>
inline T charsToInt(std::string_view s, T def, int base = 10)
{
    if (s.empty() || base < 2 || base > 36) {
        return def;
    }

    T value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value, base);

    if (ec == std::errc()) {
        return value;
    }

    return def;
}

/// @brief ワイド文字列を整数型に変換する
/// @tparam T 整数型
template <std::integral T>
inline T wCharsToInt(std::wstring_view ws, T def, int base = 10)
{
    std::string s = wideCharToMultiByte(ws);
    return charsToInt<T>(s, def, base);
}

/// @brief 文字列を浮動小数点型に変換する
/// @tparam T 浮動小数点型
template <std::floating_point T>
inline T charsToFloatingPoint(std::string_view s, T def, std::chars_format fmt = std::chars_format::general)
{
    if (s.empty()) return def;

    T value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value, fmt);

    if (ec == std::errc()) {
        return value;
    }

    return def;
}

/// @brief 整数を文字列に変換する
template <std::integral T>
inline std::string intToChars(T x, std::string_view def, int base = 10)
{
    if (base < 2 || base > 36) return std::string(def);

    constexpr size_t buffer_size = std::numeric_limits<T>::digits + std::numeric_limits<T>::is_signed + 1;
    char out[buffer_size];

    auto [ptr, ec] = std::to_chars(out, out + buffer_size, x, base);

    if (ec == std::errc()) {
        return std::string(out, ptr - out);
    }

    return std::string(def);
}

/// @brief 整数をワイド文字列に変換する
template <std::integral T>
inline std::wstring intToWchars(T x, std::string_view def, int base = 10)
{
    std::string res = intToChars(x, def, base);
    return multiByteToWideChar(res);
}

/// @brief 浮動小数点数を文字列に変換する
template <std::floating_point T>
inline std::string floatingPointToChars(T x, std::string_view def, std::chars_format fmt = std::chars_format::general, int precision = -1)
{
    char out[256];

    std::to_chars_result res;
    if (precision < 0) {
        res = std::to_chars(std::begin(out), std::end(out), x, fmt);
    } else {
        res = std::to_chars(std::begin(out), std::end(out), x, fmt, precision);
    }

    if (res.ec == std::errc()) {
        return std::string(out, res.ptr - out);
    }

    return std::string(def);
}

/// @brief 浮動小数点数をワイド文字列に変換する
template <std::floating_point T>
inline std::wstring floatingPointToWchars(T x, std::string_view def, std::chars_format fmt = std::chars_format::general, int precision = -1)
{
    std::string res = floatingPointToChars(x, def, fmt, precision);
    return multiByteToWideChar(res);
}
}  // namespace str_conv

#endif  // !STR_CONV_H
