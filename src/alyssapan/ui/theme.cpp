#include "alyssapan/ui/theme.h"

#include <imgui.h>
#include <implot.h>

namespace alyssapan::ui
{
namespace
{

// The game's palette is London by night around the clock tower: soot-black stone and wet slate, and Alyssa's
// crimson as the one thing allowed to be bright, the clock's brass as the live accent. (The structure and
// contrast are YamiPan's: chrome at rest is ash and slate, the red is spent only on what is hovered, active or
// focused.) Text contrast: bone on night 15:1, ash (disabled) 4.6:1.
constexpr ImVec4 colour(int rgb, float alpha = 1.0f)
{
    return ImVec4(static_cast<float>((rgb >> 16) & 0xff) / 255.0f, static_cast<float>((rgb >> 8) & 0xff) / 255.0f,
                  static_cast<float>(rgb & 0xff) / 255.0f, alpha);
}

constexpr int kBone = 0xE4DDD3;     // body text
constexpr int kAsh = 0x7F7670;      // disabled text
constexpr int kVoid = 0x070506;     // the deepest night, behind all
constexpr int kNight = 0x0E0B0C;    // window interiors
constexpr int kSoot = 0x191314;     // title and menu bars at rest
constexpr int kWell = 0x241A1B;     // input wells and frames
constexpr int kConcrete = 0x32272A; // buttons and grabs at rest
constexpr int kRust = 0x3A3036;     // borders, separators, table rules (slate)
constexpr int kBlood = 0x7E1A2C;    // hovered
constexpr int kRedWater = 0xA3213A; // pressed / active (crimson)
constexpr int kBrass = 0xD8B46A;    // the live accent: ticks, tabs, nav
constexpr int kLantern = 0xE8D9A8;  // links and highlights

void applyStyle()
{
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 2.0f;
    style.FrameRounding = 2.0f;
    style.GrabRounding = 2.0f;
    style.PopupRounding = 2.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;

    ImVec4 *const colours = style.Colors;
    colours[ImGuiCol_Text] = colour(kBone);
    colours[ImGuiCol_TextDisabled] = colour(kAsh);
    colours[ImGuiCol_WindowBg] = colour(kNight, 0.97f);
    colours[ImGuiCol_ChildBg] = colour(kVoid, 0.35f);
    colours[ImGuiCol_PopupBg] = colour(kVoid, 0.98f);
    colours[ImGuiCol_Border] = colour(kRust, 0.90f);
    colours[ImGuiCol_BorderShadow] = colour(kVoid, 0.0f);
    colours[ImGuiCol_FrameBg] = colour(kWell, 0.95f);
    colours[ImGuiCol_FrameBgHovered] = colour(kBlood, 0.65f);
    colours[ImGuiCol_FrameBgActive] = colour(kRedWater, 0.70f);
    colours[ImGuiCol_TitleBg] = colour(kSoot);
    colours[ImGuiCol_TitleBgActive] = colour(kBlood);
    colours[ImGuiCol_TitleBgCollapsed] = colour(kSoot, 0.75f);
    colours[ImGuiCol_MenuBarBg] = colour(kSoot);
    colours[ImGuiCol_ScrollbarBg] = colour(kVoid, 0.60f);
    colours[ImGuiCol_ScrollbarGrab] = colour(kConcrete);
    colours[ImGuiCol_ScrollbarGrabHovered] = colour(kBlood);
    colours[ImGuiCol_ScrollbarGrabActive] = colour(kRedWater);
    colours[ImGuiCol_CheckMark] = colour(kBrass);
    colours[ImGuiCol_SliderGrab] = colour(kBlood);
    colours[ImGuiCol_SliderGrabActive] = colour(kBrass);
    colours[ImGuiCol_Button] = colour(kConcrete, 0.90f);
    colours[ImGuiCol_ButtonHovered] = colour(kBlood);
    colours[ImGuiCol_ButtonActive] = colour(kRedWater);
    colours[ImGuiCol_Header] = colour(kRust, 0.80f);
    colours[ImGuiCol_HeaderHovered] = colour(kBlood, 0.90f);
    colours[ImGuiCol_HeaderActive] = colour(kRedWater);
    colours[ImGuiCol_Separator] = colour(kRust);
    colours[ImGuiCol_SeparatorHovered] = colour(kBlood);
    colours[ImGuiCol_SeparatorActive] = colour(kBrass);
    colours[ImGuiCol_ResizeGrip] = colour(kRust, 0.70f);
    colours[ImGuiCol_ResizeGripHovered] = colour(kBlood, 0.90f);
    colours[ImGuiCol_ResizeGripActive] = colour(kBrass);
    colours[ImGuiCol_InputTextCursor] = colour(kBrass);
    colours[ImGuiCol_Tab] = colour(kSoot);
    colours[ImGuiCol_TabHovered] = colour(kBlood, 0.90f);
    colours[ImGuiCol_TabSelected] = colour(kRust);
    colours[ImGuiCol_TabSelectedOverline] = colour(kBrass);
    colours[ImGuiCol_TabDimmed] = colour(kNight);
    colours[ImGuiCol_TabDimmedSelected] = colour(kWell);
    colours[ImGuiCol_TabDimmedSelectedOverline] = colour(kBlood);
    colours[ImGuiCol_PlotLines] = colour(kLantern);
    colours[ImGuiCol_PlotLinesHovered] = colour(kBrass);
    colours[ImGuiCol_PlotHistogram] = colour(kBlood);
    colours[ImGuiCol_PlotHistogramHovered] = colour(kBrass);
    colours[ImGuiCol_TableHeaderBg] = colour(kWell);
    colours[ImGuiCol_TableBorderStrong] = colour(kRust);
    colours[ImGuiCol_TableBorderLight] = colour(kConcrete, 0.70f);
    colours[ImGuiCol_TableRowBg] = colour(kVoid, 0.0f);
    colours[ImGuiCol_TableRowBgAlt] = colour(kBone, 0.03f);
    colours[ImGuiCol_TextLink] = colour(kLantern);
    colours[ImGuiCol_TextSelectedBg] = colour(kRedWater, 0.55f);
    colours[ImGuiCol_DragDropTarget] = colour(kLantern);
    colours[ImGuiCol_NavCursor] = colour(kBrass);
    colours[ImGuiCol_NavWindowingHighlight] = colour(kBone, 0.70f);
}

void applyPlotStyle()
{
    ImPlotStyle &style = ImPlot::GetStyle();
    style.Colors[ImPlotCol_FrameBg] = colour(kVoid, 0.0f);
    style.Colors[ImPlotCol_PlotBg] = colour(kVoid, 0.55f);
    style.Colors[ImPlotCol_PlotBorder] = colour(kRust);
    style.Colors[ImPlotCol_LegendBg] = colour(kVoid, 0.85f);
    style.Colors[ImPlotCol_LegendBorder] = colour(kRust);
    style.Colors[ImPlotCol_LegendText] = colour(kBone);
    style.Colors[ImPlotCol_TitleText] = colour(kBone);
    style.Colors[ImPlotCol_InlayText] = colour(kBone);
    style.Colors[ImPlotCol_AxisText] = colour(kAsh);
    style.Colors[ImPlotCol_AxisGrid] = colour(kRust, 0.60f);
}

} // namespace

void applyTheme()
{
    applyStyle();
    applyPlotStyle();
}

} // namespace alyssapan::ui
