// The host's ImGui layer: a menu bar over the game (F1, or L3 + R1 on a pad) and the windows it opens.
// Everything here runs on the render thread. Each feature is a Panel: what it adds to the menu bar and, if
// it has one, its window.
#pragma once

#include <chrono>
#include <memory>
#include <vector>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>

namespace alyssapan
{
class Application;
}

namespace alyssapan::rendering
{
class Renderer;
}

namespace alyssapan::ui
{

class Panel
{
public:
    virtual ~Panel() = default;

    // Inside a menu bar: its menu, if it has one.
    virtual void drawMenu() {}

    // Each frame, whether or not the bar is open: its window, if open.
    virtual void drawWindow() {}

    // An event the panel takes for itself (a rebinding waiting for a key). Answers true when it did.
    virtual bool handleEvent(const SDL_Event &) { return false; }
};

class Overlay
{
public:
    explicit Overlay(Application &application);
    ~Overlay();

    Overlay(const Overlay &) = delete;
    Overlay &operator=(const Overlay &) = delete;

    bool initializeOnRenderThread(SDL_GPUTextureFormat targetFormat);
    void shutdownOnRenderThread();

    void processEvent(const SDL_Event &event);
    void beginFrame();
    void draw(rendering::Renderer &renderer);
    void endFrame();

    void prepareDrawData(SDL_GPUCommandBuffer *commands);
    void renderDrawData(SDL_GPUCommandBuffer *commands, SDL_GPURenderPass *pass);

private:
    bool menuChordPressed(const SDL_Event &event);

    // The display's content scale for the window, or the override in [ui] scale; applied before each
    // frame when it changed (a window dragged to another monitor is caught whichever event said so).
    float wantedUiScale() const;
    void updateUiScale();
    void applyUiScale(float scale);

    // A cursor left still over the game hides; fullscreen with the bar down it hides moving too.
    // Through ImGui, since its SDL backend re-shows the cursor every frame for the shape it wants.
    void hideIdleCursor();

    Application &m_application;
    std::vector<std::unique_ptr<Panel>> m_panels;
    bool m_initialized = false;
    bool m_frameReady = false;
    bool m_menuVisible = false;
    bool m_chordLatched = false;
    float m_uiScale = 0.0f; // the one applied; 0 before the first frame
    std::chrono::steady_clock::time_point m_lastMouseActivity = std::chrono::steady_clock::now();
};

} // namespace alyssapan::ui
