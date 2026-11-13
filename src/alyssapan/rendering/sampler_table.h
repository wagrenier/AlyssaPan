// The renderer's texture samplers (RosePan's sampler table and RosePan_RendererSetAnisotropy, as a class): one
// per (filter, wrap) pair the GS's TEX1 / CLAMP registers ask for, built at the anisotropy level the player chose
// ([display] anisotropy). Render-thread owned: update() rebuilds the table between frames when the setting moved;
// SDL_GPU keeps a released sampler alive until the command buffers that used it finish.
#pragma once

#include <array>

#include "alyssapan/gpu/device.h"

namespace alyssapan::rendering
{

class SamplerTable
{
public:
    static constexpr int kMaxAnisotropy = 16;

    explicit SamplerTable(gpu::Device &device);

    // The setting as the table holds it: clamped to 1..kMaxAnisotropy and rounded down to a power of two.
    static int levelFor(int requested);

    // Rebuilds the samplers when the requested level differs from the one they were built for.
    void update(int requested);

    SDL_GPUSampler *get(bool linear, bool repeat) const;

    int requestedLevel() const { return m_requested; }
    // The level the linear samplers run at: 1 when the device refused anisotropic samplers.
    int activeLevel() const { return m_active; }

private:
    void build(int level);

    gpu::Device &m_device;
    std::array<gpu::Sampler, 4> m_samplers;
    int m_requested = 0; // 0 = not built yet
    int m_active = 1;
};

} // namespace alyssapan::rendering
