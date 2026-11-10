// The port's root object: owns the Pan layer's services and runs the process. The main thread stays here,
// pumping the window; the game and the renderer have threads of their own (cpp-port.md "Threads").
// instance() is the one service locator, for the edge where game code, which cannot be given references,
// reaches the host. Host classes take what they need in their constructors.
#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>

#include "alyssapan/core/config.h"
#include "alyssapan/core/event_queue.h"
#include "alyssapan/core/paths.h"
#include "alyssapan/core/window.h"
#include "alyssapan/gpu/device.h"
#include "alyssapan/io/input_system.h"
#include "alyssapan/rendering/renderer.h"
#include "alyssapan/ui/overlay.h"

namespace alyssapan
{

class Application
{
public:
    explicit Application(std::span<char *> arguments);
    ~Application();

    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;

    static Application &instance();

    // Runs until the window closes. Answers the process's exit code.
    int run();

    Paths &paths() { return m_paths; }
    Config &config() { return m_config; }
    Window &window() { return *m_window; }
    gpu::Device &device() { return *m_device; }
    rendering::Renderer &renderer() { return *m_renderer; }
    io::InputSystem &input() { return *m_input; }

    // The data folder panel's answer: the game starts as soon as its files are known.
    void dataFolderChosen(const std::filesystem::path &folder);

private:
    bool startServices();
    void startGame();
    void pumpEvents();
    void updateBackgroundState();
    void quit();

    std::span<char *> m_arguments;
    Paths m_paths;
    Config m_config;
    EventQueue m_events;
    std::optional<Window> m_window;
    std::unique_ptr<gpu::Device> m_device;
    std::unique_ptr<io::InputSystem> m_input;
    std::unique_ptr<ui::Overlay> m_overlay;
    std::unique_ptr<rendering::Renderer> m_renderer;
    std::atomic<bool> m_gameStarted{false};
    bool m_running = true;
    bool m_backgroundMuted = false;
    bool m_backgroundHeld = false;
};

} // namespace alyssapan
