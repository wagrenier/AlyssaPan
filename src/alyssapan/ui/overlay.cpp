#include "alyssapan/ui/overlay.h"

#include <algorithm>
#include <cstdlib>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlgpu3.h>
#include <implot.h>

#include "alyssapan/core/application.h"
#include "alyssapan/core/logger.h"
#include "alyssapan/core/window.h"
#include "alyssapan/io/input_system.h"
#include "alyssapan/rendering/renderer.h"
#include "alyssapan/ui/panels.h"
#include "alyssapan/ui/theme.h"

namespace alyssapan::ui
{

Overlay::Overlay(Application &application) : m_application(application) {}

Overlay::~Overlay() = default;

bool Overlay::initializeOnRenderThread(SDL_GPUTextureFormat targetFormat)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    io.IniFilename = nullptr;
    applyTheme();

    if (!ImGui_ImplSDL3_InitForSDLGPU(m_application.window().get()))
    {
        log::error("ImGui's SDL3 backend failed to start");
        return false;
    }

    ImGui_ImplSDLGPU3_InitInfo info{};
    info.Device = m_application.device().get();
    info.ColorTargetFormat = targetFormat;
    info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    if (!ImGui_ImplSDLGPU3_Init(&info))
    {
        log::error("ImGui's SDL_GPU backend failed to start");
        return false;
    }

    m_panels = createPanels(m_application);
    m_menuVisible = std::getenv("ALYSSAPAN_SHOW_MENU") != nullptr;
    m_initialized = true;
    return true;
}

void Overlay::shutdownOnRenderThread()
{
    if (!m_initialized)
        return;
    m_panels.clear();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    m_initialized = false;
}

// L3 + R1 going down together on one pad, in either order. Read from the pad's state, since the event
// names only one of the two; the latch stops both downs of one press from toggling the bar twice. Fixed
// buttons, not the game's bindings, the same as F1 is a fixed key, and they still reach the game.
bool Overlay::menuChordPressed(const SDL_Event &event)
{
    if (event.type != SDL_EVENT_GAMEPAD_BUTTON_DOWN && event.type != SDL_EVENT_GAMEPAD_BUTTON_UP)
        return false;
    if (event.gbutton.button != SDL_GAMEPAD_BUTTON_LEFT_STICK &&
        event.gbutton.button != SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)
    {
        return false;
    }

    SDL_Gamepad *const gamepad = SDL_GetGamepadFromID(event.gbutton.which);
    const bool held = event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && gamepad != nullptr &&
                      SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_STICK) &&
                      SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
    const bool pressed = held && !m_chordLatched;
    m_chordLatched = held;
    return pressed;
}

void Overlay::processEvent(const SDL_Event &event)
{
    if (!m_initialized)
        return;

    for (const std::unique_ptr<Panel> &panel : m_panels)
    {
        if (panel->handleEvent(event))
            return;
    }

    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
    {
        if (event.key.scancode == SDL_SCANCODE_F1)
            m_menuVisible = !m_menuVisible;
        else if (event.key.scancode == SDL_SCANCODE_RETURN && (event.key.mod & SDL_KMOD_ALT) != 0)
            m_application.window().toggleFullscreen();
    }
    if (menuChordPressed(event))
        m_menuVisible = !m_menuVisible;

    if (event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        event.type == SDL_EVENT_MOUSE_WHEEL)
    {
        m_lastMouseActivity = std::chrono::steady_clock::now();
    }

    ImGui_ImplSDL3_ProcessEvent(&event);
}

float Overlay::wantedUiScale() const
{
    const float configured = m_application.config().ui.scale;
    if (configured > 0.0f)
        return configured;

    // The content scale over the pixel density: with SDL_WINDOW_HIGH_PIXEL_DENSITY a Retina-style
    // window already has its pixels doubled, and ImGui's own coordinates are the window's.
    SDL_Window *const window = m_application.window().get();
    const float display = SDL_GetWindowDisplayScale(window);
    float density = SDL_GetWindowPixelDensity(window);
    if (!(display > 0.0f))
        return 1.0f; // SDL answers 0 on failure
    if (!(density > 0.0f))
        density = 1.0f;
    return std::clamp(display / density, kUiScaleMin, kUiScaleMax);
}

void Overlay::updateUiScale()
{
    const float wanted = wantedUiScale();
    if (wanted != m_uiScale)
        applyUiScale(wanted);
}

// Both styles rebuilt from their defaults at `scale`, so scaling twice never compounds.
void Overlay::applyUiScale(float scale)
{
    ImGuiStyle &style = ImGui::GetStyle();
    style = ImGuiStyle();
    ImPlot::GetStyle() = ImPlotStyle();
    applyTheme();
    style.ScaleAllSizes(scale);
    style.FontScaleDpi = scale;

    // ImPlot has no ScaleAllSizes: its pixel sizes by hand (alphas and fractions stay).
    ImPlotStyle &plot = ImPlot::GetStyle();
    for (ImVec2 *size : {&plot.PlotDefaultSize, &plot.PlotMinSize, &plot.MajorTickLen, &plot.MinorTickLen,
                         &plot.MajorTickSize, &plot.MinorTickSize, &plot.MajorGridSize, &plot.MinorGridSize,
                         &plot.PlotPadding, &plot.LabelPadding, &plot.LegendPadding, &plot.LegendInnerPadding,
                         &plot.LegendSpacing, &plot.MousePosPadding, &plot.AnnotationPadding})
    {
        size->x *= scale;
        size->y *= scale;
    }
    plot.PlotBorderSize *= scale;
    plot.DigitalBitHeight *= scale;
    plot.DigitalBitGap *= scale;

    log::info("overlay scale {:.2f}", scale);
    m_uiScale = scale;
}

void Overlay::hideIdleCursor()
{
    if (ImGui::GetIO().WantCaptureMouse)
        return;
    const float idleSeconds = m_application.config().ui.cursorHideSeconds;
    const bool idle = idleSeconds > 0.0f && std::chrono::steady_clock::now() - m_lastMouseActivity >=
                                                std::chrono::duration<float>(idleSeconds);
    const bool fullscreenPlay = m_application.window().mode() == WindowMode::Borderless && !m_menuVisible;
    if (idle || fullscreenPlay)
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
}

void Overlay::beginFrame()
{
    if (!m_initialized)
        return;
    updateUiScale();
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    m_frameReady = false;
}

void Overlay::draw(rendering::Renderer &)
{
    if (!m_initialized)
        return;

    if (m_menuVisible && ImGui::BeginMainMenuBar())
    {
        for (const std::unique_ptr<Panel> &panel : m_panels)
            panel->drawMenu();
        ImGui::EndMainMenuBar();
    }
    for (const std::unique_ptr<Panel> &panel : m_panels)
        panel->drawWindow();
    hideIdleCursor();

    // The game reads a released pad while a window here has the keyboard.
    m_application.input().setSuppressed(ImGui::GetIO().WantCaptureKeyboard);
}

void Overlay::endFrame()
{
    if (!m_initialized)
        return;
    ImGui::Render();
    m_frameReady = true;
}

void Overlay::prepareDrawData(SDL_GPUCommandBuffer *commands)
{
    if (m_frameReady)
        ImGui_ImplSDLGPU3_PrepareDrawData(ImGui::GetDrawData(), commands);
}

void Overlay::renderDrawData(SDL_GPUCommandBuffer *commands, SDL_GPURenderPass *pass)
{
    if (m_frameReady)
        ImGui_ImplSDLGPU3_RenderDrawData(ImGui::GetDrawData(), commands, pass);
}

} // namespace alyssapan::ui
