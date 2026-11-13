#include "alyssapan/rendering/msaa_targets.h"

#include <algorithm>

#include "alyssapan/core/logger.h"

namespace alyssapan::rendering
{
namespace
{

SDL_GPUSampleCount sdlSampleCount(int samples)
{
    switch (samples)
    {
    case 8:
        return SDL_GPU_SAMPLECOUNT_8;
    case 4:
        return SDL_GPU_SAMPLECOUNT_4;
    case 2:
        return SDL_GPU_SAMPLECOUNT_2;
    default:
        return SDL_GPU_SAMPLECOUNT_1;
    }
}

} // namespace

MsaaTargets::MsaaTargets(gpu::Device &device, SDL_GPUTextureFormat colorFormat, SDL_GPUTextureFormat depthFormat)
    : m_device(device), m_colorFormat(colorFormat), m_depthFormat(depthFormat)
{
    for (int samples = 2; samples <= kMaxSamples; samples *= 2)
    {
        const SDL_GPUSampleCount count = sdlSampleCount(samples);
        if (!SDL_GPUTextureSupportsSampleCount(m_device.get(), m_colorFormat, count) ||
            !SDL_GPUTextureSupportsSampleCount(m_device.get(), m_depthFormat, count))
            break; // counts are supported downward, not in gaps
        m_maxSupported = samples;
    }
    log::info("MSAA: up to {}x on this device", m_maxSupported);
}

int MsaaTargets::countFor(int requested)
{
    int samples = 1;
    while (samples * 2 <= std::clamp(requested, 1, kMaxSamples))
        samples *= 2;
    return samples;
}

bool MsaaTargets::update(int requested)
{
    const int count = countFor(requested);
    if (count == m_requested)
        return false;

    const int previous = m_active;
    m_requested = count;
    m_active = std::min(count, m_maxSupported);
    if (m_active != count)
        log::warning("MSAA: {}x requested, the device grants {}x", count, m_active);
    else
        log::info("MSAA: {}x", m_active);
    return m_active != previous;
}

SDL_GPUSampleCount MsaaTargets::sampleCount() const
{
    return sdlSampleCount(m_active);
}

void MsaaTargets::ensure(int width, int height)
{
    if (m_active <= 1)
    {
        // Off: give the memory back rather than park it for the session (RosePan).
        m_color = {};
        m_depth = {};
        m_targetSamples = 1;
        return;
    }
    if (m_color && m_width == width && m_height == height && m_targetSamples == m_active)
        return;

    // COLOR_TARGET only: a multisampled texture cannot be sampled, which is why the pass resolves.
    m_color = m_device.createTexture(static_cast<unsigned>(width), static_cast<unsigned>(height), m_colorFormat,
                                     SDL_GPU_TEXTUREUSAGE_COLOR_TARGET, sampleCount());
    m_depth = m_device.createTexture(static_cast<unsigned>(width), static_cast<unsigned>(height), m_depthFormat,
                                     SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET, sampleCount());
    m_width = width;
    m_height = height;
    m_targetSamples = m_active;
    if (!m_color || !m_depth)
    {
        // Refused at this size: run without, and say so in the menu.
        log::warning("MSAA: no {}x targets at {}x{}; running at 1x", m_active, width, height);
        m_color = {};
        m_depth = {};
        m_active = 1;
        m_targetSamples = 1;
    }
}

} // namespace alyssapan::rendering
