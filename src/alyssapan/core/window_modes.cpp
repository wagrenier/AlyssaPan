// The window's modes and placement: windowed, borderless and exclusive fullscreen, the saved position, the
// size presets. Carried over from MikuPan (mikupan_renderer.cpp's window modes, ui/mikupan_ui_settings.cpp's
// presets and centring) as Window members; saving the position across runs is new here.
#include "alyssapan/core/window.h"

#include <algorithm>
#include <array>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_stdinc.h>

#include "alyssapan/core/logger.h"

namespace alyssapan
{

namespace
{

// How much of the title bar must still be on a display for a saved position to be used: enough to grab
// and drag the window back. A monitor unplugged since the last run leaves less, and the window is centred.
constexpr int kGrabWidth = 64;
constexpr int kGrabHeight = 24;

// The usable area (no taskbar) of the display the window is on, or of the primary one.
bool displayBounds(SDL_Window *window, SDL_Rect &bounds)
{
    SDL_DisplayID display = SDL_GetDisplayForWindow(window);
    if (display == 0)
        display = SDL_GetPrimaryDisplay();
    if (display == 0)
        return false;
    return SDL_GetDisplayUsableBounds(display, &bounds) || SDL_GetDisplayBounds(display, &bounds);
}

bool titleBarOnSomeDisplay(int x, int y, int width)
{
    int count = 0;
    SDL_DisplayID *const displays = SDL_GetDisplays(&count);
    if (displays == nullptr)
        return false;
    const SDL_Rect grab{x, y, std::min(width, kGrabWidth), kGrabHeight};
    bool found = false;
    for (int i = 0; i < count && !found; i++)
    {
        SDL_Rect bounds;
        SDL_Rect overlap;
        if (SDL_GetDisplayUsableBounds(displays[i], &bounds) && SDL_GetRectIntersection(&grab, &bounds, &overlap))
            found = overlap.w == grab.w && overlap.h == grab.h;
    }
    SDL_free(displays);
    return found;
}

constexpr std::array kSizePresets{
    WindowSizePreset{640, 480, "640 x 480 (4:3, the PS2's picture)"},
    WindowSizePreset{960, 720, "960 x 720 (4:3)"},
    WindowSizePreset{1280, 960, "1280 x 960 (4:3, 2x)"},
    WindowSizePreset{1440, 1080, "1440 x 1080 (4:3)"},
    WindowSizePreset{1280, 720, "1280 x 720 (16:9)"},
    WindowSizePreset{1600, 900, "1600 x 900 (16:9)"},
    WindowSizePreset{1920, 1080, "1920 x 1080 (16:9)"},
    WindowSizePreset{1680, 1050, "1680 x 1050 (16:10)"},
    WindowSizePreset{2560, 1080, "2560 x 1080 (21:9)"},
};

} // namespace

std::span<const WindowSizePreset> windowSizePresets()
{
    return kSizePresets;
}

void Window::setMode(WindowMode mode)
{
    runOnMainThread([this, mode] {
        // Borderless takes no video mode of its own, so alt-tab is instant and the desktop stays as it is.
        // Exclusive asks for the desktop's own mode, so the picture keeps its size but the driver owns the
        // screen (MikuPan's choice: a lower mode is the render resolution's job, not the window's).
        const SDL_DisplayMode *videoMode = nullptr;
        if (mode == WindowMode::Exclusive)
        {
            videoMode = SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(m_window));
            if (videoMode == nullptr)
                log::warning("no desktop mode for exclusive fullscreen ({}): borderless instead", SDL_GetError());
        }
        if (mode != WindowMode::Windowed && !SDL_SetWindowFullscreenMode(m_window, videoMode))
            log::warning("SDL_SetWindowFullscreenMode: {}", SDL_GetError());
        bool done = SDL_SetWindowFullscreen(m_window, mode != WindowMode::Windowed);
        // A display that cannot change modes refuses exclusive fullscreen outright: Remote Desktop answers
        // DISP_CHANGE_FAILED (seen 2026-11-13). Borderless needs no mode change, so the player still gets
        // a fullscreen picture; the setting keeps asking for exclusive on the next start.
        if (!done && videoMode != nullptr)
        {
            log::warning("exclusive fullscreen refused ({}): borderless instead", SDL_GetError());
            SDL_SetWindowFullscreenMode(m_window, nullptr);
            done = SDL_SetWindowFullscreen(m_window, true);
            if (done)
            {
                SDL_SyncWindow(m_window);
                m_mode = m_fullscreenMode = WindowMode::Borderless;
                return;
            }
        }
        if (!done)
        {
            log::error("SDL_SetWindowFullscreen: {}", SDL_GetError());
            return;
        }
        // The change lands some frames later otherwise, and the next frame would be recorded at a size
        // thrown away a frame on.
        SDL_SyncWindow(m_window);
        m_mode = mode;
        if (mode != WindowMode::Windowed)
            m_fullscreenMode = mode;
    });
}

void Window::toggleFullscreen()
{
    setMode(m_mode == WindowMode::Windowed ? m_fullscreenMode : WindowMode::Windowed);
}

void Window::setWindowedSize(int width, int height)
{
    if (m_mode != WindowMode::Windowed)
        setMode(WindowMode::Windowed);
    runOnMainThread([this, width, height] {
        SDL_RestoreWindow(m_window);
        if (!SDL_SetWindowSize(m_window, width, height))
        {
            log::warning("SDL_SetWindowSize: {}", SDL_GetError());
            return;
        }
        SDL_SyncWindow(m_window);
    });
    centre();
}

void Window::centre()
{
    runOnMainThread([this] {
        if (m_mode != WindowMode::Windowed)
            return;
        SDL_Rect bounds;
        if (!displayBounds(m_window, bounds))
            return;
        int width = 0;
        int height = 0;
        SDL_GetWindowSize(m_window, &width, &height);
        // A window larger than the display keeps its top-left corner on it, so the title bar stays in reach.
        const int x = std::max(bounds.x, bounds.x + (bounds.w - width) / 2);
        const int y = std::max(bounds.y, bounds.y + (bounds.h - height) / 2);
        SDL_SetWindowPosition(m_window, x, y);
        SDL_SyncWindow(m_window);
    });
}

bool Window::fitsDisplay(int width, int height) const
{
    SDL_Rect bounds;
    if (!displayBounds(m_window, bounds))
        return true;
    return width <= bounds.w && height <= bounds.h;
}

void Window::place(int x, int y)
{
    if (x != kWindowUnplaced && y != kWindowUnplaced && titleBarOnSomeDisplay(x, y, m_windowedWidth))
    {
        runOnMainThread([this, x, y] {
            SDL_SetWindowPosition(m_window, x, y);
            SDL_SyncWindow(m_window);
        });
    }
    else
    {
        if (x != kWindowUnplaced)
            log::info("saved window position {}, {} is off every display: centred", x, y);
        centre();
    }
    runOnMainThread([this] { SDL_GetWindowPosition(m_window, &m_windowedX, &m_windowedY); });
    log::info("window {} x {} at {}, {}", m_windowedWidth, m_windowedHeight, m_windowedX, m_windowedY);
}

bool Window::keepsPlacement() const
{
    if (m_mode != WindowMode::Windowed)
        return false;
    const SDL_WindowFlags flags = SDL_GetWindowFlags(m_window);
    return (flags & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_MINIMIZED | SDL_WINDOW_FULLSCREEN)) == 0;
}

} // namespace alyssapan
