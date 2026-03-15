#pragma once

#include <charconv>
#include <concepts>
#include <cstdint>
#include <string>

#include "alias_parser.h"
#include "utils/common/str_conv.h"

#include "plugin2.h"

namespace plugin2_utils {

/// @brief get_object_item_value() で値を設定する際のデフォルト値やセクションの指定、基数変換などできるようにしたもの。設定値は内部で文字列に変換してから設定する。
/// @tparam T セットする値の型
/// @param edit 編集セクション構造体
/// @param object オブジェクトハンドル
/// @param effect_name 対象のエフェクト名
/// @param effect_index 対象のエフェクトのインデックス
/// @param item_name 対象の設定項目の名称
/// @param value セットする値
/// @param default_value セットする値のデフォルト値
/// @param section_index セクションのインデックス
/// @param base value が整数の場合の出力基数。2進数から36進数まで
/// @param fmt value が浮動小数点数の場合の出力フォーマット
template <typename T>
void setObjectItemValue(const EDIT_SECTION* edit, const OBJECT_HANDLE object, const wchar_t* effect_name, const uint32_t effect_index, const wchar_t* item_name, const T& value, T default_value, const uint32_t section_index = 0, const int32_t base = 10, const std::chars_format fmt = std::chars_format::general)
{
    if (edit->count_object_effect(object, effect_name) <= 0) return;

    std::wstring effect_with_index = std::wstring{effect_name} + L":" + str_conv::intToWchars(effect_index, "0");
    auto ret_ptr                   = edit->get_object_item_value(object, effect_with_index.c_str(), item_name);
    if (!ret_ptr) {
        return;
    }
    std::string ret_str = ret_ptr;

    std::string set_value_str{};
    if constexpr (std::integral<T>) {
        set_value_str = str_conv::intToChars(value, str_conv::intToChars(default_value, "0", base), base);
    } else if constexpr (std::floating_point<T>) {
        set_value_str = str_conv::floatingPointToChars(value, str_conv::floatingPointToChars(default_value, "0", fmt), fmt);
    } else if constexpr (std::constructible_from<T, const char*>) {
        set_value_str = std::string{value};
    } else {
        set_value_str = default_value;
    }

    // key=value1,valu2,value3... のとき、引数 index に基づいていずれかの値のみを置き換える
    std::string replaced_str = alias_parser::replaceNthToken(ret_str, section_index, set_value_str);
    edit->set_object_item_value(object, effect_with_index.c_str(), item_name, replaced_str.c_str());
}

/// @brief get_object_item_value() で値を取得する際のデフォルト値やセクションの指定、基数変換などできるようにしたもの。設定値は内部でデフォルト値の型 T に変換してから返す。
/// @tparam T デフォルト値の型
/// @param edit 編集セクション構造体
/// @param object オブジェクトハンドル
/// @param effect_name 対象のエフェクト名
/// @param effect_index 対象のエフェクトのインデックス
/// @param item_name 対象の設定項目の名称
/// @param default_value 取得する値のデフォルト値。値が取得できなかった場合に返される
/// @param section_index セクションのインデックス
/// @param base 取得する値の整数の基数。2進数から36進数まで
/// @param fmt 取得する値の浮動小数点数のフォーマット指定
/// @return
template <typename T>
T getObjectItemValue(const EDIT_SECTION* edit, const OBJECT_HANDLE object, const wchar_t* effect_name, const uint32_t effect_index, const wchar_t* item_name, const T default_value, const uint32_t section_index = 0, const int32_t base = 10, const std::chars_format fmt = std::chars_format::general)
{
    if (edit->count_object_effect(object, effect_name) <= 0) return default_value;

    std::wstring effect_with_index = std::wstring{effect_name} + L":" + str_conv::intToWchars(effect_index, "0");
    auto ret_ptr                   = edit->get_object_item_value(object, effect_with_index.c_str(), item_name);
    if (!ret_ptr) {
        return default_value;
    }

    std::string ret_str = ret_ptr;
    ret_str             = alias_parser::getNthToken(ret_str, section_index);

    if constexpr (std::integral<T>) {
        return str_conv::charsToInt(ret_str, default_value, base);
    } else if constexpr (std::floating_point<T>) {
        return str_conv::charsToFloatingPoint(ret_str, default_value, fmt);
    } else if constexpr (std::constructible_from<T, const char*>) {
        return std::string{ret_str};
    }
}

/// @brief キャプチャ付きのラムダ式を call_edit_section_param() に渡すためのヘルパー関数
/// @tparam F ラムダ式
/// @param api_func call_edit_section_param() の関数ポインタ
/// @param lambda 実行したいラムダ式
/// @return call_edit_section_param() の実行結果。trueなら成功
template <typename F>
bool call_edit_lambda(bool (*api_func)(void*, void (*)(void*, EDIT_SECTION*)), F&& lambda)
{
    auto callback_shim = [](void* param, EDIT_SECTION* edit) {
        auto* func = static_cast<std::remove_reference_t<F>*>(param);
        (*func)(edit);
    };
    return api_func((void*)&lambda, callback_shim);
}

}  // namespace plugin2_utils
