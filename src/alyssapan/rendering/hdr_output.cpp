#include "alyssapan/rendering/hdr_output.h"

#include <array>
#include <utility>

#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>

#include "alyssapan/core/logger.h"
#include "alyssapan/core/window.h"

namespace alyssapan::rendering
{
namespace
{

// scRGB's definition: 1.0 in extended-linear sRGB is 80 cd/m2.
constexpr float kScrgbWhiteNits = 80.0f;
// PQ's 1.0 is 10000 cd/m2.
constexpr float kPqMaxNits = 10000.0f;
// Paper white when neither the setting nor the display gives one.
constexpr float kDefaultPaperWhiteNits = 200.0f;

// present.frag.hlsl's OUTPUT_* modes.
constexpr float kOutputSdr = 0.0f;
constexpr float kOutputScRgb = 1.0f;
constexpr float kOutputPq = 2.0f;

// One code value of each target: 8-bit SDR, 10-bit HDR10; scRGB's half floats need no dither.
constexpr float kDitherSdr = 1.0f / 255.0f;
constexpr float kDitherPq = 1.0f / 1023.0f;

// The expansion curve's shoulder: RosePan's fixed 2 (a slider here would be a knob with no picture to judge).
constexpr float kExpansionShoulder = 2.0f;

} // namespace

HdrOutput::HdrOutput(gpu::Device &device, Window &window, const Config &config)
    : m_device(device), m_window(window), m_config(config)
{
    refreshDisplay();
}

void HdrOutput::refreshDisplay()
{
    const SDL_PropertiesID props = SDL_GetWindowProperties(m_window.get());
    if (props == 0)
        return;

    m_displayHdrEnabled = SDL_GetBooleanProperty(props, SDL_PROP_WINDOW_HDR_ENABLED_BOOLEAN, false);
    m_displaySdrWhite = SDL_GetFloatProperty(props, SDL_PROP_WINDOW_SDR_WHITE_LEVEL_FLOAT, 1.0f);
    m_displayHeadroom = SDL_GetFloatProperty(props, SDL_PROP_WINDOW_HDR_HEADROOM_FLOAT, 1.0f);
    if (!(m_displaySdrWhite > 0.0f))
        m_displaySdrWhite = 1.0f;
    if (!(m_displayHeadroom >= 1.0f))
        m_displayHeadroom = 1.0f;
}

float HdrOutput::displaySdrWhiteNits() const
{
    // Apple platforms answer 1.0 (80 nits, scRGB's own white); Windows the level the player set.
    return m_displaySdrWhite * kScrgbWhiteNits;
}

float HdrOutput::paperWhiteNits() const
{
    const float setting = m_config.display.hdrPaperWhite;
    if (setting > 0.0f)
        return setting;
    const float nits = displaySdrWhiteNits();
    return nits > 0.0f ? nits : kDefaultPaperWhiteNits;
}

float HdrOutput::peakNits() const
{
    // Never below paper white: a lower peak would make the expansion darken highlights.
    const float paper = paperWhiteNits();
    float peak = m_config.display.hdrPeak;
    if (!(peak > 0.0f))
        peak = displaySdrWhiteNits() * m_displayHeadroom;
    return peak > paper ? peak : paper;
}

bool HdrOutput::isHdr(SDL_GPUSwapchainComposition composition)
{
    return composition == SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR ||
           composition == SDL_GPU_SWAPCHAINCOMPOSITION_HDR10_ST2084;
}

const char *HdrOutput::compositionName(SDL_GPUSwapchainComposition composition)
{
    switch (composition)
    {
    case SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR:
        return "SDR linear";
    case SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR:
        return "HDR scRGB";
    case SDL_GPU_SWAPCHAINCOMPOSITION_HDR10_ST2084:
        return "HDR10 PQ";
    case SDL_GPU_SWAPCHAINCOMPOSITION_SDR:
        break;
    }
    return "SDR";
}

// Auto asks the display first: no HDR on it (an SDR monitor, or HDR off in the OS) keeps SDR, since an HDR
// swapchain there shows a washed-out picture rather than failing. A named mode skips that test, but every
// candidate must still pass SDL_WindowSupportsGPUSwapchainComposition; SDR ends every list.
SDL_GPUSwapchainComposition HdrOutput::resolve() const
{
    const HdrMode mode = m_config.display.hdrMode;
    if (mode == HdrMode::Off || (mode == HdrMode::Auto && !m_displayHdrEnabled))
        return SDL_GPU_SWAPCHAINCOMPOSITION_SDR;

    std::array<SDL_GPUSwapchainComposition, 2> candidates{SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR,
                                                          SDL_GPU_SWAPCHAINCOMPOSITION_HDR10_ST2084};
    if (mode == HdrMode::Hdr10)
        std::swap(candidates[0], candidates[1]);
    for (const SDL_GPUSwapchainComposition candidate : candidates)
    {
        if (SDL_WindowSupportsGPUSwapchainComposition(m_device.get(), m_window.get(), candidate))
            return candidate;
    }
    return SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
}

void HdrOutput::apply(SDL_GPUPresentMode presentMode)
{
    const SDL_GPUSwapchainComposition wanted = resolve();
    SDL_GPUSwapchainComposition running = wanted;
    if (!SDL_SetGPUSwapchainParameters(m_device.get(), m_window.get(), wanted, presentMode))
    {
        gpu::logGpuError("SDL_SetGPUSwapchainParameters");
        if (wanted == SDL_GPU_SWAPCHAINCOMPOSITION_SDR)
            return;
        // Back to something known to work rather than whatever the failed call left.
        running = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
        if (!SDL_SetGPUSwapchainParameters(m_device.get(), m_window.get(), running, presentMode))
        {
            gpu::logGpuError("SDL_SetGPUSwapchainParameters(SDR)");
            return;
        }
    }

    if (!m_applied || running != m_composition)
    {
        log::info("Swapchain: {} (display HDR {}, SDR white {:.0f} nits, headroom {:.2f}x)",
                  compositionName(running), m_displayHdrEnabled ? "on" : "off", displaySdrWhiteNits(),
                  m_displayHeadroom);
    }
    m_composition = running;
    m_applied = true;
}

void HdrOutput::fillUniforms(float hdr[4], float output[4]) const
{
    const float paper = paperWhiteNits();
    output[2] = kExpansionShoulder;
    output[3] = 0.0f;

    switch (m_composition)
    {
    case SDL_GPU_SWAPCHAINCOMPOSITION_HDR_EXTENDED_LINEAR:
        output[0] = kOutputScRgb;
        output[1] = 0.0f;
        hdr[0] = paper / kScrgbWhiteNits;
        break;
    case SDL_GPU_SWAPCHAINCOMPOSITION_HDR10_ST2084:
        output[0] = kOutputPq;
        output[1] = kDitherPq;
        hdr[0] = paper / kPqMaxNits;
        break;
    case SDL_GPU_SWAPCHAINCOMPOSITION_SDR:
    case SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR:
        output[0] = kOutputSdr;
        output[1] = kDitherSdr;
        hdr[0] = 1.0f;
        break;
    }

    if (active())
    {
        hdr[1] = peakNits() / paper;
        hdr[2] = m_config.display.hdrExpansion;
        hdr[3] = m_config.display.hdrExpansionKnee;
    }
    else
    {
        // An SDR present is the grade and the dither: no headroom to expand into.
        hdr[1] = 1.0f;
        hdr[2] = 0.0f;
        hdr[3] = 0.0f;
    }
}

} // namespace alyssapan::rendering
