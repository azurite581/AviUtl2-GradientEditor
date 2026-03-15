#ifndef IMGUI_STYLE_H
#define IMGUI_SYYLE_H

#include "config2_utils.h"
#include "color_conv.h"

#include "imgui.h"

namespace gradient_editor {
    inline void applyCustomColors() {
        //
        // スタイル設定
        //
        ImGuiStyle& style = ImGui::GetStyle();

        auto aulColor2imVec4 = [](const std::string& name) -> ImVec4 {
            return color_conv::u32Rgb2Vec4Rgba<ImVec4>(aul2::getColor(name.c_str()));
        };

        // ウィンドウ
        style.Colors[ImGuiCol_WindowBg] = aulColor2imVec4("Grouping");
        style.Colors[ImGuiCol_PopupBg]  = aulColor2imVec4("Grouping");
        style.Colors[ImGuiCol_Border]   = aulColor2imVec4("Border");

        // テキスト
        style.Colors[ImGuiCol_Text] = aulColor2imVec4("Text");

        // ボタン & フレーム
        style.Colors[ImGuiCol_Button]         = aulColor2imVec4("ButtonBody");
        style.Colors[ImGuiCol_ButtonHovered]  = aulColor2imVec4("ButtonBodyHover");
        style.Colors[ImGuiCol_ButtonActive]   = aulColor2imVec4("ButtonBodyPress");
        style.Colors[ImGuiCol_FrameBg]        = aulColor2imVec4("ButtonBody");
        style.Colors[ImGuiCol_FrameBgHovered] = aulColor2imVec4("ButtonBodyHover");
        style.Colors[ImGuiCol_FrameBgActive]  = aulColor2imVec4("ButtonBodySelect");

        // メニューバー
        style.Colors[ImGuiCol_MenuBarBg] = aulColor2imVec4("TitleHeader");

        // タブ
        style.Colors[ImGuiCol_TitleBg]             = aulColor2imVec4("Background");
        style.Colors[ImGuiCol_TitleBgActive]       = aulColor2imVec4("Background");
        style.Colors[ImGuiCol_Tab]                 = aulColor2imVec4("GroupingHover");
        style.Colors[ImGuiCol_TabDimmed]           = aulColor2imVec4("GroupingHover");
        style.Colors[ImGuiCol_TabDimmedSelected]   = aulColor2imVec4("GroupingSelect");
        style.Colors[ImGuiCol_TabSelected]         = aulColor2imVec4("GroupingSelect");
        style.Colors[ImGuiCol_TabHovered]          = aulColor2imVec4("GroupingSelect");
        style.Colors[ImGuiCol_TabSelectedOverline] = aulColor2imVec4("BorderFocus");

        // コンボボックスの選択している項目 & ホバー中の項目
        style.Colors[ImGuiCol_Header]        = aulColor2imVec4("ButtonBodySelect");
        style.Colors[ImGuiCol_HeaderHovered] = aulColor2imVec4("ButtonBodySelect");
        style.Colors[ImGuiCol_HeaderActive]  = aulColor2imVec4("ButtonBodySelect");

        // スライダー
        style.Colors[ImGuiCol_SliderGrab]       = aulColor2imVec4("SliderCursor");
        style.Colors[ImGuiCol_SliderGrabActive] = aulColor2imVec4("SliderCursor");
    }
}

#endif
