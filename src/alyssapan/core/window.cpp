#include "alyssapan/core/window.h"

#include <format>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_surface.h>

#include "alyssapan/core/logger.h"
#include "alyssapan/core/paths.h"

namespace alyssapan
{

void runOnMainThread(const std::function<void()> &work)
{
    const auto call = [](void *data) { (*static_cast<const std::function<void()> *>(data))(); };
    if (!SDL_RunOnMainThread(call, const_cast<std::function<void()> *>(&work), true))
        log::error("SDL_RunOnMainThread: {}", SDL_GetError());
}

std::expected<Window, std::string> Window::create(const DisplaySettings &settings)
{
    SDL_Window *const window = SDL_CreateWindow("AlyssaPan", settings.windowWidth, settings.windowHeight,
                                                SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == nullptr)
        return std::unexpected(std::format("SDL_CreateWindow: {}", SDL_GetError()));

    Window result(window, settings);
    result.place(settings.windowX, settings.windowY);
    result.setMode(settings.windowMode);
    return result;
}

Window::Window(SDL_Window *window, const DisplaySettings &settings)
    : m_window(window), m_mode(WindowMode::Windowed),
      m_fullscreenMode(settings.windowMode == WindowMode::Exclusive ? WindowMode::Exclusive
                                                                    : WindowMode::Borderless),
      m_windowedWidth(settings.windowWidth), m_windowedHeight(settings.windowHeight),
      m_windowedX(kWindowUnplaced), m_windowedY(kWindowUnplaced)
{
}

Window::Window(Window &&other) noexcept
    : m_window(std::exchange(other.m_window, nullptr)), m_mode(other.m_mode),
      m_fullscreenMode(other.m_fullscreenMode), m_windowedWidth(other.m_windowedWidth),
      m_windowedHeight(other.m_windowedHeight), m_windowedX(other.m_windowedX), m_windowedY(other.m_windowedY)
{
}

Window::~Window()
{
    if (m_window != nullptr)
        SDL_DestroyWindow(m_window);
}

std::pair<int, int> Window::pixelSize() const
{
    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(m_window, &width, &height);
    return {width, height};
}

void Window::noteResized()
{
    if (!keepsPlacement())
        return;
    SDL_GetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);
}

void Window::noteMoved()
{
    if (!keepsPlacement())
        return;
    SDL_GetWindowPosition(m_window, &m_windowedX, &m_windowedY);
}

void Window::setIcon(const std::filesystem::path &png)
{
    const std::string file = pathToUtf8(png);
    SDL_Surface *const icon = SDL_LoadPNG(file.c_str());
    if (icon == nullptr)
    {
        log::warning("window icon {}: {}", file, SDL_GetError());
        return;
    }
    runOnMainThread([this, icon] {
        if (!SDL_SetWindowIcon(m_window, icon))
            log::warning("SDL_SetWindowIcon: {}", SDL_GetError());
    });
    SDL_DestroySurface(icon);
}

bool Window::inBackground() const
{
    const SDL_WindowFlags flags = SDL_GetWindowFlags(m_window);
    return (flags & SDL_WINDOW_MINIMIZED) != 0 || (flags & SDL_WINDOW_INPUT_FOCUS) == 0;
}

} // namespace alyssapan
