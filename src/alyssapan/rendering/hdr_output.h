// The HDR swapchain (RosePan's rosepan_renderer.cpp, as a class): what the window's display can do, which
// swapchain composition the player's [display] hdr setting gets on it, and the numbers PresentPass needs to
// write the finished SDR picture in that encoding. The source stays SDR: HDR shows it at a chosen paper white
// and may lift its highlights into the display's headroom (present.frag.hlsl says how).
//
// Render-thread owned after construction: the renderer refreshes it on SDL_EVENT_WINDOW_HDR_STATE_CHANGED (the
// window moved to another display, or HDR was switched in the OS) and applies it with the present mode.
#pragma once

#include <SDL3/SDL_gpu.h>

#include "alyssapan/core/config.h"
#include "alyssapan/gpu/device.h"

namespace alyssapan
{
class Window;
}

namespace alyssapan::rendering
{

class HdrOutput
{
public:
    HdrOutput(gpu::Device &device, Window &window, const Config &config);

    // Re-reads the window's HDR state, SDR white level and headroom.
    void refreshDisplay();

    // Sets the swapchain to the composition the setting resolves to, with this present mode; falls back to SDR
    // when the window refuses it. Logs the composition when it changes.
    void apply(SDL_GPUPresentMode presentMode);

    SDL_GPUSwapchainComposition composition() const { return m_composition; }
    bool active() const { return isHdr(m_composition); }
    bool displayHdrEnabled() const { return m_displayHdrEnabled; }
    float displayHeadroom() const { return m_displayHeadroom; }
    float displaySdrWhiteNits() const;
    float paperWhiteNits() const;
    float peakNits() const;

    // PresentUniforms' hdr and output vectors (present.frag.hlsl) for the running composition.
    void fillUniforms(float hdr[4], float output[4]) const;

    static bool isHdr(SDL_GPUSwapchainComposition composition);
    static const char *compositionName(SDL_GPUSwapchainComposition composition);

private:
    SDL_GPUSwapchainComposition resolve() const;

    gpu::Device &m_device;
    Window &m_window;
    const Config &m_config;

    SDL_GPUSwapchainComposition m_composition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    bool m_applied = false;
    bool m_displayHdrEnabled = false;
    float m_displaySdrWhite = 1.0f; // in scRGB units (1.0 = 80 nits)
    float m_displayHeadroom = 1.0f;
};

} // namespace alyssapan::rendering
