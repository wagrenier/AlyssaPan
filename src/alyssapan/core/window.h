// The OS window. Created on the main thread, which also pumps its messages: on Windows a drag or resize
// runs a modal loop inside the pump, so nothing else may live on that thread. State changes are posted to
// it from wherever they come and waited for.
#pragma once

#include <expected>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <utility>

#include <SDL3/SDL_video.h>

#include "alyssapan/core/config.h"

namespace alyssapan
{

// Runs `work` on the main thread and waits for it (inline when already there).
void runOnMainThread(const std::function<void()> &work);

class Window
{
public:
    static std::expected<Window, std::string> create(const DisplaySettings &settings);

    Window(Window &&other) noexcept;
    Window &operator=(Window &&) = delete;
    ~Window();

    SDL_Window *get() const { return m_window; }

    // Its size now, in pixels. Any thread.
    std::pair<int, int> pixelSize() const;

    WindowMode mode() const { return m_mode; }
    bool fullscreen() const { return m_mode != WindowMode::Windowed; }
    void setMode(WindowMode mode);
    // Alt+Enter: windowed, or back to whichever fullscreen was used last (borderless at first).
    void toggleFullscreen();

    // A size preset: leaves fullscreen, un-maximises, takes the size and centres the window on its display.
    void setWindowedSize(int width, int height);
    // Centres the windowed window on the display it is on.
    void centre();
    // Whether a windowed size fits the usable area of the window's display (a preset that does not would
    // put the title bar off the screen).
    bool fitsDisplay(int width, int height) const;

    // The size to come back to: while fullscreen the live size is the desktop's, and saving that
    // would lose the window's shape for good.
    std::pair<int, int> windowedSize() const { return {m_windowedWidth, m_windowedHeight}; }

    // Where the windowed window last stood; kWindowUnplaced until known.
    std::pair<int, int> windowedPosition() const { return {m_windowedX, m_windowedY}; }

    // From the event pump, on a resize or a move.
    void noteResized();
    void noteMoved();

    // The title bar's and taskbar's picture, from a PNG (the exe's own comes from extern/icon.rc). A
    // missing file leaves the system's default and says so in the log.
    void setIcon(const std::filesystem::path &png);

    // Minimised, or without the keyboard focus: nobody is playing. Read off the window's flags rather
    // than tracked from focus events, so a missed or coalesced event cannot leave it stuck (MioPan).
    bool inBackground() const;

private:
    Window(SDL_Window *window, const DisplaySettings &settings);

    // At start-up: the saved position when part of the window would still be on a display, else centred.
    void place(int x, int y);
    // Only a plain window's own size and place are kept: a maximised one's are the display's.
    bool keepsPlacement() const;

    SDL_Window *m_window;
    WindowMode m_mode;
    WindowMode m_fullscreenMode;
    int m_windowedWidth;
    int m_windowedHeight;
    int m_windowedX;
    int m_windowedY;
};

// The size presets the settings menu offers: the PS2's 4:3 picture at whole and common scales, then the
// usual wide shapes (any aspect is the port's first feature, so the window may take any of them).
struct WindowSizePreset
{
    int width;
    int height;
    const char *label;
};
std::span<const WindowSizePreset> windowSizePresets();

} // namespace alyssapan
