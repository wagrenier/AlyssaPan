// The scene's multisampled targets (RosePan's EnsureMsaaColorTexture / RosePan_RendererSetMsaaSamples, as a
// class): at [display] msaa > 1 the game's frame is rasterised into a multisampled colour target and depth target
// and resolved into the scene texture at the end of the pass, so everything that reads the scene (the output blit,
// the screenshots) never knows MSAA exists. Render-thread owned: update() picks the count between frames.
//
// The count is the request rounded down to a power of two and clamped to the largest count the device grants for
// both the scene's colour format and its depth format (counts are supported downward, not in gaps: RosePan).
// A change of count means new targets and new pipelines (a pipeline bakes its sample count); the renderer drops
// its pipeline cache and redraws the shown frame when update() reports one.
#pragma once

#include <SDL3/SDL_gpu.h>

#include "alyssapan/gpu/device.h"

namespace alyssapan::rendering
{

class MsaaTargets
{
public:
    static constexpr int kMaxSamples = 8;

    MsaaTargets(gpu::Device &device, SDL_GPUTextureFormat colorFormat, SDL_GPUTextureFormat depthFormat);

    // The setting as the targets hold it: clamped to 1..kMaxSamples and rounded down to a power of two.
    static int countFor(int requested);

    // Picks the active count for the request; true when it changed (pipelines and targets are stale).
    bool update(int requested);

    // (Re)creates the multisampled targets for the scene's size; nothing at one sample.
    void ensure(int width, int height);

    bool active() const { return m_active > 1; }
    int requestedCount() const { return m_requested; }
    // The count the scene runs at: the request, or less when the device grants less.
    int activeCount() const { return m_active; }
    int maxSupported() const { return m_maxSupported; }
    SDL_GPUSampleCount sampleCount() const;

    SDL_GPUTexture *color() const { return m_color.get(); }
    SDL_GPUTexture *depth() const { return m_depth.get(); }

private:
    gpu::Device &m_device;
    SDL_GPUTextureFormat m_colorFormat;
    SDL_GPUTextureFormat m_depthFormat;
    gpu::Texture m_color;
    gpu::Texture m_depth;
    int m_width = 0;
    int m_height = 0;
    int m_targetSamples = 1; // the count m_color / m_depth were made at
    int m_requested = 0;     // 0 = not chosen yet
    int m_active = 1;
    int m_maxSupported = 1;
};

} // namespace alyssapan::rendering
