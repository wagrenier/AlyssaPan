#include "alyssapan/rendering/frame_pacing.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace alyssapan::rendering
{
namespace
{

// A refresh-to-frame ratio this far from a whole number holds a frame an extra refresh every 100 game frames
// or more often (60 Hz against the game's 59.94-field clock is 0.002: one every 500).
constexpr double kUnevenTolerance = 0.01;

// A present interval this long is a stall (a load, a dragged window, a screenshot), not a refresh: no display
// refreshes below 10 Hz.
constexpr double kStallMs = 100.0;

// hold() rounds the refreshes per game frame down, with this much slack: a display a little slower than a
// whole multiple (59.9 Hz under the game's 59.94-field clock: 1.998) still holds 2, and the renderer's
// catch-up (a second frame waiting) absorbs the difference.
constexpr double kHoldSlack = 0.1;
constexpr unsigned kMaxHold = 8;

double toMs(FramePacing::Clock::duration duration)
{
    return std::chrono::duration<double, std::milli>(duration).count();
}

template <std::size_t N>
double median(const std::array<float, N> &values, std::size_t count)
{
    if (count == 0)
        return 0.0;
    std::vector<float> sorted(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(std::min(count, N)));
    const auto middle = sorted.begin() + static_cast<std::ptrdiff_t>(sorted.size() / 2);
    std::nth_element(sorted.begin(), middle, sorted.end());
    return *middle;
}

template <std::size_t N>
void push(std::array<float, N> &ring, std::size_t &cursor, std::size_t &count, double value)
{
    ring[cursor] = static_cast<float>(value);
    cursor = (cursor + 1) % N;
    count = std::min(count + 1, N);
}

} // namespace

void FramePacing::presented(Clock::time_point when, const Clock::time_point *field)
{
    if (m_havePresent)
        push(m_presentMs, m_presentCursor, m_presentCount, toMs(when - m_lastPresent));
    m_lastPresent = when;
    m_havePresent = true;

    if (field == nullptr)
        return;
    if (m_haveFrame)
    {
        push(m_onScreenMs, m_cursor, m_count, toMs(when - m_lastNewFrame));
        if (*field > m_lastField)
            push(m_gameMs, m_gameCursor, m_gameCount, toMs(*field - m_lastField));
    }
    m_lastNewFrame = when;
    m_lastField = *field;
    m_haveFrame = true;
}

FramePacing::Summary FramePacing::summary() const
{
    Summary summary;
    const std::size_t frames = std::min(m_count, kWindow);
    if (frames > 0)
    {
        double sum = 0.0;
        double squares = 0.0;
        for (std::size_t back = 1; back <= frames; back++)
        {
            const double value = m_onScreenMs[(m_cursor + kHistory - back) % kHistory];
            sum += value;
            squares += value * value;
        }
        summary.meanMs = sum / static_cast<double>(frames);
        summary.deviationMs =
            std::sqrt(std::max(0.0, squares / static_cast<double>(frames) - summary.meanMs * summary.meanMs));
    }
    // A median: a load's long frame would pull a mean off the game's cadence.
    summary.gameMs = median(m_gameMs, m_gameCount);
    // A mean, stalls left out, not a median: a swapchain that releases its images in bursts (a Remote Desktop
    // session's) presents in pairs, a few ms then most of two refreshes apart, and only the mean is the rate.
    double presentSum = 0.0;
    std::size_t presents = 0;
    for (std::size_t index = 0; index < m_presentCount; index++)
    {
        if (m_presentMs[index] < kStallMs)
        {
            presentSum += m_presentMs[index];
            presents++;
        }
    }
    summary.refreshHz = presentSum > 0.0 ? 1000.0 * static_cast<double>(presents) / presentSum : 0.0;
    return summary;
}

unsigned FramePacing::hold() const
{
    const Summary current = summary();
    if (current.gameMs <= 0.0 || current.refreshHz <= 0.0)
        return 1;
    const double refreshesPerFrame = current.gameMs * current.refreshHz / 1000.0;
    return std::clamp(static_cast<unsigned>(refreshesPerFrame + kHoldSlack), 1u, kMaxHold);
}

bool FramePacing::uneven(const Summary &summary)
{
    if (summary.gameMs <= 0.0 || summary.refreshHz <= 0.0)
        return false;
    const double refreshesPerFrame = summary.gameMs * summary.refreshHz / 1000.0;
    return refreshesPerFrame < 1.0 - kUnevenTolerance ||
           std::abs(refreshesPerFrame - std::round(refreshesPerFrame)) > kUnevenTolerance;
}

} // namespace alyssapan::rendering
