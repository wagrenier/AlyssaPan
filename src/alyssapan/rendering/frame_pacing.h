// What the player sees of the game's frame rate: how long each game frame stayed on screen, from the
// present that first showed it to the present that replaced it. The game makes a frame every second field
// (29.97 a second, gaWaitVSync's swap), timed by the interrupt controller's clock, which runs apart from the
// display's; the render thread shows them in order, each for hold() refreshes (Renderer::renderOnce). On a
// display whose refresh is a multiple of the game's rate (60 Hz: 2 refreshes a frame, 120 Hz: 4) every frame
// stays an even 33.4 ms, but for one refresh more every ~17 s as the two clocks drift; on one that is not (144
// Hz, or the 32 Hz a Remote Desktop session composes at) some frames stay a refresh longer than the rest, and
// that unevenness is what reads as stutter at a steady 30 fps. The Frame graph plots these times; a frame
// rate averaged over half a second hides them. Render thread only (the overlay draws there).
#pragma once

#include <array>
#include <chrono>
#include <cstddef>

namespace alyssapan::rendering
{

class FramePacing
{
public:
    using Clock = std::chrono::steady_clock;

    static constexpr std::size_t kHistory = 600; // game frames: 20 s at 30 fps

    // One present, just submitted. `field`: when the field that released the game frame it brought was due
    // (Frame::time), or null for a repeat of the frame already on screen.
    void presented(Clock::time_point when, const Clock::time_point *field);

    // Each shown game frame's time on screen in ms, a ring: count() values, the oldest at offset().
    const std::array<float, kHistory> &onScreenMs() const { return m_onScreenMs; }
    std::size_t offset() const { return m_count < kHistory ? 0 : m_cursor; }
    std::size_t count() const { return m_count; }

    struct Summary
    {
        double meanMs = 0.0;      // time on screen, over the recent frames
        double deviationMs = 0.0; // its standard deviation: 0 is perfectly even
        double gameMs = 0.0;      // the game's own frame interval (field times): what an even display shows
        double refreshHz = 0.0;   // the presents' rate, stalls left out: the display's refresh under VSync
    };
    // Over the last kWindow game frames and presents; zeros until there are some.
    Summary summary() const;

    // The display's refresh is no whole multiple of the game's frame rate, so frames cannot all stay on
    // screen equally long (meaningful under VSync, where presents follow the display).
    static bool uneven(const Summary &summary);

    // Under VSync every present stays on screen for one refresh, so presents count refreshes: how many each
    // game frame should get before the next replaces it, the whole refreshes one game frame spans (2 at 60 Hz,
    // 4 at 120, 1 at a Remote Desktop session's 32). 1 until the rates are known.
    unsigned hold() const;

private:
    static constexpr std::size_t kWindow = 150;       // game frames (5 s) the summary covers
    static constexpr std::size_t kPresentWindow = 64; // presents the refresh is measured over

    std::array<float, kHistory> m_onScreenMs{};
    std::size_t m_cursor = 0;
    std::size_t m_count = 0;

    std::array<float, kWindow> m_gameMs{};
    std::size_t m_gameCursor = 0;
    std::size_t m_gameCount = 0;

    std::array<float, kPresentWindow> m_presentMs{};
    std::size_t m_presentCursor = 0;
    std::size_t m_presentCount = 0;

    Clock::time_point m_lastPresent{};
    Clock::time_point m_lastNewFrame{};
    Clock::time_point m_lastField{};
    bool m_havePresent = false;
    bool m_haveFrame = false;
};

} // namespace alyssapan::rendering
