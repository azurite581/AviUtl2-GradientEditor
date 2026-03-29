#ifndef IMGUI_UTILS_H
#define IMGUI_UTILS_H

#include <string>
#include <vector>

#include "imgui.h"
#include "imgui_internal.h"

namespace imgui_utils {
// 次に配置する要素の位置を指定する
// https://github.com/ocornut/imgui/discussions/3862#discussioncomment-422097
inline void alignForWidth(float width, float alignment = 0.5f)
{
    float avail = ImGui::GetContentRegionAvail().x;
    float off   = (avail - width) * alignment;
    if (off > 0.0f)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);
}

// スピンコントロール
// https://github.com/ocornut/imgui/issues/2649#issue-464385323
inline bool spinScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_step, const void* p_step_fast, const char* format, ImGuiInputTextFlags flags)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g   = *GImGui;
    ImGuiStyle& style = g.Style;
    IM_ASSERT((flags & ImGuiInputTextFlags_EnterReturnsTrue) == 0);  // Not supported by InputScalar(). Please open an issue if you this would be useful to you. Otherwise use IsItemDeactivatedAfterEdit()!

    if (format == NULL)
        format = ImGui::DataTypeGetInfo(data_type)->PrintFmt;

    void* p_data_default = (g.NextItemData.HasFlags & ImGuiNextItemDataFlags_HasRefVal) ? &g.NextItemData.RefVal : &g.DataTypeZeroValue;

    char buf[64];
    if ((flags & ImGuiInputTextFlags_DisplayEmptyRefVal) && ImGui::DataTypeCompare(data_type, p_data, p_data_default) == 0)
        buf[0] = 0;
    else
        ImGui::DataTypeFormatString(buf, IM_ARRAYSIZE(buf), data_type, p_data, format);

    // Disable the MarkItemEdited() call in InputText but keep ImGuiItemStatusFlags_Edited.
    // We call MarkItemEdited() ourselves by comparing the actual data rather than the string.
    g.NextItemData.ItemFlags |= ImGuiItemFlags_NoMarkEdited;
    flags |= ImGuiInputTextFlags_AutoSelectAll | (ImGuiInputTextFlags)ImGuiInputTextFlags_LocalizeDecimalPoint;

    bool value_changed = false;
    if (p_step == NULL) {
        if (ImGui::InputText(label, buf, IM_ARRAYSIZE(buf), flags))
            value_changed = ImGui::DataTypeApplyFromText(buf, data_type, p_data, format, (flags & ImGuiInputTextFlags_ParseEmptyRefVal) ? p_data_default : NULL);
    } else {
        const float button_size = ImGui::GetFrameHeight();

        ImGui::BeginGroup();  // The only purpose of the group here is to allow the caller to query item data e.g. IsItemActive()
        ImGui::PushID(label);
        ImGui::SetNextItemWidth(ImMax(1.0f, ImGui::CalcItemWidth() - (button_size + style.ItemInnerSpacing.x) * 2));
        if (ImGui::InputText("", buf, IM_ARRAYSIZE(buf), flags))  // PushId(label) + "" gives us the expected ID from outside point of view
            value_changed = ImGui::DataTypeApplyFromText(buf, data_type, p_data, format, (flags & ImGuiInputTextFlags_ParseEmptyRefVal) ? p_data_default : NULL);
        IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Inputable);

        // Step buttons
        const ImVec2 backup_frame_padding = style.FramePadding;
        style.FramePadding.x              = style.FramePadding.y;
        if (flags & ImGuiInputTextFlags_ReadOnly)
            ImGui::BeginDisabled();
        ImGui::PushItemFlag(ImGuiItemFlags_ButtonRepeat, true);
        ImGui::SameLine(0.0f, 0.0f);

        float frame_height  = ImGui::GetFrameHeight();
        float arrow_size    = frame_height * 0.5f;
        float arrow_spacing = 0.0f;

        ImGui::BeginGroup();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{g.Style.ItemSpacing.x, arrow_spacing});

        // save/change font size to draw arrow buttons correctly
        float org_font_size                      = ImGui::GetDrawListSharedData()->FontSize;
        ImGui::GetDrawListSharedData()->FontSize = arrow_size;

        if (ImGui::ArrowButtonEx("+", ImGuiDir_Up, ImVec2(arrow_size, arrow_size))) {
            ImGui::DataTypeApplyOp(data_type, '+', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
            value_changed = true;
        }

        if (ImGui::ArrowButtonEx("-", ImGuiDir_Down, ImVec2(arrow_size, arrow_size))) {
            ImGui::DataTypeApplyOp(data_type, '-', p_data, p_data, g.IO.KeyCtrl && p_step_fast ? p_step_fast : p_step);
            value_changed = true;
        }

        ImGui::PopItemFlag();
        if (flags & ImGuiInputTextFlags_ReadOnly)
            ImGui::EndDisabled();

        // restore font size
        ImGui::GetDrawListSharedData()->FontSize = org_font_size;

        ImGui::PopStyleVar(1);
        ImGui::EndGroup();

        const char* label_end = ImGui::FindRenderedTextEnd(label);
        if (label != label_end) {
            ImGui::SameLine(0, style.ItemInnerSpacing.x);
            ImGui::TextEx(label, label_end);
        }
        style.FramePadding = backup_frame_padding;

        ImGui::PopID();
        ImGui::EndGroup();
    }

    g.LastItemData.ItemFlags &= ~ImGuiItemFlags_NoMarkEdited;
    if (value_changed)
        ImGui::MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

inline bool spinInt(const char* label, int* v, int step = 1, int step_fast = 100, ImGuiInputTextFlags flags = 0)
{
    // Hexadecimal input provided as a convenience but the flag name is awkward. Typically you'd use InputText() to parse your own data, if you want to handle prefixes.
    const char* format = (flags & ImGuiInputTextFlags_CharsHexadecimal) ? "%08X" : "%d";
    return spinScalar(label, ImGuiDataType_S32, (void*)v, (void*)(step > 0 ? &step : NULL), (void*)(step_fast > 0 ? &step_fast : NULL), format, flags);
}

// ボタントグル
inline bool pushToggleButton(const char* label, bool* v, const ImVec2& size = ImVec2(0, 0))
{
    if (*v) {
        ImVec4 active_color = ImGui::GetStyle().Colors[ImGuiCol_ButtonActive];
        ImGui::PushStyleColor(ImGuiCol_Button, active_color);
    }

    bool pressed = ImGui::Button(label, size);
    if (*v) ImGui::PopStyleColor();
    if (pressed) *v = !(*v);

    return pressed;
}

// アイコンフォトント用の正方形のボタン
inline bool squareIconButton(const std::string& icon, const std::string& label)
{
    ImGui::PushStyleVarX(ImGuiStyleVar_FramePadding, (ImGui::GetFrameHeight() - ImGui::CalcTextSize(icon.c_str()).x) * 0.5f);
    bool f = ImGui::Button((icon + label).data());
    ImGui::PopStyleVar();
    return f;
}

inline static void helpMarker(const char* icon, const char* desc)
{
    ImGui::TextDisabled(icon);
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

}  // namespace imgui_utils

#endif
