#include "alyssapan/core/profiler.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <format>
#include <string>
#include <utility>

#include "alyssapan/core/logger.h"

namespace alyssapan
{

namespace
{

constexpr std::size_t index(Profiler::Phase phase)
{
    return static_cast<std::size_t>(phase);
}

constexpr std::size_t index(Profiler::Lane lane)
{
    return static_cast<std::size_t>(lane);
}

double toMs(Profiler::Clock::duration elapsed)
{
    return std::chrono::duration<double, std::milli>(elapsed).count();
}

} // namespace

std::string_view Profiler::name(Phase phase)
{
    switch (phase)
    {
    case Phase::FieldWait:
        return "field wait";
    case Phase::EeIdle:
        return "EE idle";
    case Phase::Textures:
        return "textures";
    case Phase::FrameKick:
        return "frame kick";
    case Phase::Overlay:
        return "overlay";
    case Phase::Upload:
        return "upload";
    case Phase::Record:
        return "record";
    case Phase::SwapchainAcquire:
        return "swapchain wait";
    case Phase::Submit:
        return "submit";
    }
    return "?";
}

std::string_view Profiler::name(Lane lane)
{
    return lane == Lane::Game ? "game" : "render";
}

Profiler::Profiler(std::filesystem::path stutterLog, double stutterMs)
    : m_stutterPath(std::move(stutterLog)), m_stutterMs(stutterMs > 0.0 ? stutterMs : 0.0)
{
    if (m_stutterMs > 0.0)
        log::info("Stutter log: frames working longer than {:.1f} ms go to {}", m_stutterMs, m_stutterPath.string());
}

void Profiler::addPhase(Phase phase, Clock::duration elapsed)
{
    const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
    m_lanes[index(laneOf(phase))].current[index(phase)].fetch_add(nanoseconds, std::memory_order_relaxed);
}

void Profiler::endFrame(Lane lane, std::string_view note)
{
    LaneState &state = m_lanes[index(lane)];
    const Clock::time_point now = Clock::now();
    const double frameMs = toMs(now - state.frameStart);
    state.frameStart = now;
    state.frameNumber++;

    std::array<double, kPhaseCount> phaseMs{};
    double waitMs = 0.0;
    for (std::size_t i = 0; i < kPhaseCount; i++)
    {
        const auto phase = static_cast<Phase>(i);
        if (laneOf(phase) != lane)
            continue;
        phaseMs[i] = static_cast<double>(state.current[i].exchange(0, std::memory_order_relaxed)) / 1.0e6;
        if (isWait(phase))
            waitMs += phaseMs[i];
    }
    // A wait that began in the previous frame can end past this frame's start: never below zero.
    const double workMs = frameMs > waitMs ? frameMs - waitMs : 0.0;

    Totals &totals = state.totals;
    totals.frames++;
    totals.frameMs += frameMs;
    totals.frameMaxMs = std::max(totals.frameMaxMs, frameMs);
    totals.workMs += workMs;
    totals.workMaxMs = std::max(totals.workMaxMs, workMs);
    for (std::size_t i = 0; i < kPhaseCount; i++)
    {
        totals.phaseMs[i] += phaseMs[i];
        totals.phaseMaxMs[i] = std::max(totals.phaseMaxMs[i], phaseMs[i]);
    }

    if (m_stutterMs > 0.0 && workMs >= m_stutterMs)
        writeStutter(lane, state.frameNumber, frameMs, workMs, phaseMs, note);

    if (now - state.windowStart < kWindow)
        return;

    LaneStats published;
    const double frames = static_cast<double>(totals.frames);
    published.frames = totals.frames;
    published.frame = {totals.frameMs / frames, totals.frameMaxMs};
    published.work = {totals.workMs / frames, totals.workMaxMs};
    for (std::size_t i = 0; i < kPhaseCount; i++)
        published.phases[i] = {totals.phaseMs[i] / frames, totals.phaseMaxMs[i]};
    {
        std::scoped_lock lock(m_statsLock);
        m_published.lanes[index(lane)] = published;
    }
    totals = {};
    state.windowStart = now;
}

Profiler::Stats Profiler::stats() const
{
    std::scoped_lock lock(m_statsLock);
    return m_published;
}

void Profiler::writeStutter(Lane lane, std::uint64_t frameNumber, double frameMs, double workMs,
                            const std::array<double, kPhaseCount> &phaseMs, std::string_view note)
{
    std::scoped_lock lock(m_stutterLock);
    if (m_stutterOpenFailed)
        return;
    if (!m_stutterFile.is_open())
    {
        // In the user directory, not the working directory: RosePan learnt that the latter is whatever the
        // launcher or debugger set, and not writable at all once installed.
        m_stutterFile.open(m_stutterPath, std::ios::app);
        if (!m_stutterFile)
        {
            m_stutterOpenFailed = true;
            log::warning("Stutter log: cannot open {}", m_stutterPath.string());
            return;
        }
        const std::time_t started = std::time(nullptr);
        char stamp[32] = {};
        std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", std::localtime(&started));
        m_stutterFile << std::format("# run {}, threshold {:.1f} ms of work\n", stamp, m_stutterMs);
    }

    std::string line = std::format("{} frame {}: work {:.2f} ms, frame {:.2f} ms", name(lane), frameNumber, workMs,
                                   frameMs);
    for (std::size_t i = 0; i < kPhaseCount; i++)
    {
        const auto phase = static_cast<Phase>(i);
        if (laneOf(phase) == lane && phaseMs[i] >= 0.01)
            line += std::format(" | {} {:.2f}", name(phase), phaseMs[i]);
    }
    if (!note.empty())
        line += std::format(" | {}", note);
    line += '\n';
    m_stutterFile << line;
    m_stutterFile.flush(); // a hitch before a crash is the one worth reading
}

} // namespace alyssapan
