// Where a frame's time goes, for the Performance panel and for stutter hunting (RosePan's rosepan_profiler,
// cpp-port.md "To carry over" 12). RosePan ran the game and the renderer on one thread; here they have one
// each, so the profiler keeps two lanes: the game thread's frame runs from one UiCanvas kick to the next, the
// render thread's from one present to the next. Each lane times its phases; what is left of the frame after
// its waits is the lane's work, the number a slow machine runs out of.
//
// A stutter threshold ([log] stutter_ms, ALYSSAPAN_STUTTER_MS; 0 = off) appends every frame whose work exceeds
// it to alyssapan_stutter.log in the user directory, one line per frame with each phase's share, so a hitch
// says which work it did, not only how long it took. The threshold is on the work, not the frame: the game
// waits for its fields and the renderer for the swapchain, so every frame's wall time is about a field. The
// game's wait is gaWaitVSync's spin (the ROM's busy loop on a flag its VSync callback clears, so the EE never
// idles while it waits) plus any stretch with no EE thread running (ee::Kernel::setIdleObserver).
#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

namespace alyssapan
{

class Profiler
{
public:
    using Clock = std::chrono::steady_clock;

    enum class Lane
    {
        Game,
        Render,
    };
    static constexpr std::size_t kLaneCount = 2;

    enum class Phase
    {
        // The game thread.
        FieldWait,  // gaWaitVSync's spin until the VSync callback releases it (AlyssaPan_FieldWaitBegin)
        EeIdle,     // no EE thread runs: every one waits (the vblank, an interrupt, a semaphore)
        Textures,   // texture lookups, a first sight decoding its TIM2
        FrameKick,  // UiCanvas::kick: the frame's layers gathered and handed over
        // The render thread.
        Overlay,          // ImGui's frame built
        Upload,           // images and vertices copied to the GPU
        Record,           // the scene and output passes recorded
        SwapchainAcquire, // waiting for a swapchain image (the present pacing)
        Submit,           // the command buffer submitted
    };
    static constexpr std::size_t kPhaseCount = 9;

    static constexpr Lane laneOf(Phase phase) { return phase < Phase::Overlay ? Lane::Game : Lane::Render; }
    // The phases that wait rather than work: they are taken out of the lane's work.
    static constexpr bool isWait(Phase phase) { return phase == Phase::FieldWait || phase == Phase::EeIdle || phase == Phase::SwapchainAcquire; }
    static std::string_view name(Phase phase);
    static std::string_view name(Lane lane);

    struct Metric
    {
        double meanMs = 0.0; // per frame of the lane, over the window
        double maxMs = 0.0;  // the worst single frame in the window
    };
    struct LaneStats
    {
        int frames = 0; // in the window
        Metric frame;
        Metric work;
        std::array<Metric, kPhaseCount> phases{};
    };
    struct Stats
    {
        std::array<LaneStats, kLaneCount> lanes{};
    };

    // Times one phase on the calling thread's lane for as long as it lives.
    class Scope
    {
    public:
        Scope(Profiler &profiler, Phase phase) : m_profiler(profiler), m_phase(phase), m_start(Clock::now()) {}
        ~Scope() { m_profiler.addPhase(m_phase, Clock::now() - m_start); }
        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;

    private:
        Profiler &m_profiler;
        Phase m_phase;
        Clock::time_point m_start;
    };

    Profiler(std::filesystem::path stutterLog, double stutterMs);
    Profiler(const Profiler &) = delete;
    Profiler &operator=(const Profiler &) = delete;

    // From any thread; lands in the phase's lane's current frame.
    void addPhase(Phase phase, Clock::duration elapsed);

    // Closes the lane's current frame (on that lane's thread). The note goes into a stutter line: what the
    // frame carried (draws, vertices, a repeat).
    void endFrame(Lane lane, std::string_view note = {});

    // The last complete window (about half a second) of each lane.
    Stats stats() const;

    double stutterThresholdMs() const { return m_stutterMs; }

private:
    static constexpr auto kWindow = std::chrono::milliseconds(500);

    struct Totals
    {
        int frames = 0;
        double frameMs = 0.0, frameMaxMs = 0.0;
        double workMs = 0.0, workMaxMs = 0.0;
        std::array<double, kPhaseCount> phaseMs{};
        std::array<double, kPhaseCount> phaseMaxMs{};
    };
    struct LaneState
    {
        // Nanoseconds of the frame in progress, per phase: written by whichever thread runs the phase.
        std::array<std::atomic<std::int64_t>, kPhaseCount> current{};
        Clock::time_point frameStart = Clock::now();
        Clock::time_point windowStart = Clock::now();
        std::uint64_t frameNumber = 0;
        Totals totals;
    };

    void writeStutter(Lane lane, std::uint64_t frameNumber, double frameMs, double workMs,
                      const std::array<double, kPhaseCount> &phaseMs, std::string_view note);

    std::array<LaneState, kLaneCount> m_lanes;
    mutable std::mutex m_statsLock;
    Stats m_published;

    std::filesystem::path m_stutterPath;
    double m_stutterMs;
    std::mutex m_stutterLock;
    std::ofstream m_stutterFile;
    bool m_stutterOpenFailed = false;
};

} // namespace alyssapan
