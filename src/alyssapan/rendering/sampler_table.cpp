#include "alyssapan/rendering/sampler_table.h"

#include <algorithm>

#include "alyssapan/core/logger.h"

namespace alyssapan::rendering
{

namespace
{

constexpr std::size_t indexOf(bool linear, bool repeat)
{
    return (linear ? 1u : 0u) | (repeat ? 2u : 0u);
}

} // namespace

SamplerTable::SamplerTable(gpu::Device &device) : m_device(device)
{
}

int SamplerTable::levelFor(int requested)
{
    const int clamped = std::clamp(requested, 1, kMaxAnisotropy);
    int level = 1;
    while (level * 2 <= clamped)
        level *= 2;
    return level;
}

void SamplerTable::update(int requested)
{
    const int level = levelFor(requested);
    if (level == m_requested)
        return;
    build(level);
    m_requested = level;
    log::info("Texture anisotropy: {}x requested, {}x active", level, m_active);
}

void SamplerTable::build(int level)
{
    m_active = level;
    for (bool repeat : {false, true})
    {
        for (bool linear : {false, true})
        {
            const SDL_GPUSamplerAddressMode address =
                repeat ? SDL_GPU_SAMPLERADDRESSMODE_REPEAT : SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
            const SDL_GPUFilter filter = linear ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;

            SDL_GPUSamplerCreateInfo info{};
            info.min_filter = filter;
            info.mag_filter = filter;
            info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
            info.address_mode_u = address;
            info.address_mode_v = address;
            info.address_mode_w = address;

            // Only on an all-linear filter (RosePan's rule, hazards.md): SDL's D3D12 backend ORs the anisotropic
            // bit onto the basic filter, and D3D12 defines exactly one anisotropic value (MIN_MAG_MIP_LINEAR plus
            // the bit); any other mix makes a value that is no D3D12_FILTER, CreateSampler reports nothing and the
            // device fails every later call. A NEAREST filter is also the GS asking for point sampling.
            const bool anisotropic = linear && m_active > 1;
            if (anisotropic)
            {
                info.min_filter = SDL_GPU_FILTER_LINEAR;
                info.mag_filter = SDL_GPU_FILTER_LINEAR;
                info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
                info.enable_anisotropy = true;
                info.max_anisotropy = static_cast<float>(m_active);
            }

            gpu::Sampler sampler = m_device.createSampler(info);
            if (!sampler && anisotropic)
            {
                // A device without anisotropic sampling: plain samplers, and the menu shows 1x active.
                log::warning("Anisotropic samplers refused; texture anisotropy is off");
                info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
                info.enable_anisotropy = false;
                info.max_anisotropy = 0.0f;
                sampler = m_device.createSampler(info);
                m_active = 1;
            }
            m_samplers[indexOf(linear, repeat)] = std::move(sampler);
        }
    }
}

SDL_GPUSampler *SamplerTable::get(bool linear, bool repeat) const
{
    return m_samplers[indexOf(linear, repeat)].get();
}

} // namespace alyssapan::rendering
