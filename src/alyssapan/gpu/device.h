// RAII over SDL_GPU. A Handle releases its object through the device when it goes; Device owns the
// SDL_GPUDevice and what every pass needs from it: shaders from the shipped bytecode, and uploads.
#pragma once

#include <cstddef>
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <SDL3/SDL_gpu.h>

namespace alyssapan
{
class Paths;
}

namespace alyssapan::gpu
{

template <class Object, void(SDLCALL *Release)(SDL_GPUDevice *, Object *)>
class Handle
{
public:
    Handle() = default;
    Handle(SDL_GPUDevice *device, Object *object) : m_device(device), m_object(object) {}
    ~Handle() { reset(); }

    Handle(Handle &&other) noexcept
        : m_device(other.m_device), m_object(std::exchange(other.m_object, nullptr))
    {
    }

    Handle &operator=(Handle &&other) noexcept
    {
        if (this != &other)
        {
            reset();
            m_device = other.m_device;
            m_object = std::exchange(other.m_object, nullptr);
        }
        return *this;
    }

    void reset()
    {
        if (m_object != nullptr)
            Release(m_device, std::exchange(m_object, nullptr));
    }

    Object *get() const { return m_object; }
    explicit operator bool() const { return m_object != nullptr; }

private:
    SDL_GPUDevice *m_device = nullptr;
    Object *m_object = nullptr;
};

using Texture = Handle<SDL_GPUTexture, SDL_ReleaseGPUTexture>;
using Buffer = Handle<SDL_GPUBuffer, SDL_ReleaseGPUBuffer>;
using TransferBuffer = Handle<SDL_GPUTransferBuffer, SDL_ReleaseGPUTransferBuffer>;
using Sampler = Handle<SDL_GPUSampler, SDL_ReleaseGPUSampler>;
using Shader = Handle<SDL_GPUShader, SDL_ReleaseGPUShader>;
using GraphicsPipeline = Handle<SDL_GPUGraphicsPipeline, SDL_ReleaseGPUGraphicsPipeline>;

struct ShaderResources
{
    unsigned samplers = 0;
    unsigned uniformBuffers = 0;
    unsigned storageBuffers = 0;
};

class Device
{
public:
    // Creates the device on `driver` (an SDL driver name; empty, or one that fails, lets SDL choose), with the
    // backend's validation layer when `debug`. The window is claimed by claimWindow, on the window's thread.
    static std::expected<std::unique_ptr<Device>, std::string> create(const Paths &paths, std::string_view driver,
                                                                      bool debug);

    // The backend that runs ("direct3d12", "vulkan", "metal").
    std::string_view driverName() const;

    ~Device();
    Device(const Device &) = delete;
    Device &operator=(const Device &) = delete;

    SDL_GPUDevice *get() const { return m_device; }

    // Loads resources/shaders/<format>/<name>.<ext>, in the format the backend takes. `resources`
    // has to be exact: SDL expects every declared slot to be bound before a draw.
    Shader loadShader(std::string_view name, SDL_GPUShaderStage stage, ShaderResources resources) const;

    // samples > 1 makes a multisampled target (COLOR_TARGET / DEPTH_STENCIL_TARGET usage only).
    Texture createTexture(unsigned width, unsigned height, SDL_GPUTextureFormat format,
                          SDL_GPUTextureUsageFlags usage, SDL_GPUSampleCount samples = SDL_GPU_SAMPLECOUNT_1) const;
    Sampler createSampler(const SDL_GPUSamplerCreateInfo &info) const;
    TransferBuffer createUploadBuffer(unsigned size) const;
    TransferBuffer createDownloadBuffer(unsigned size) const;
    Buffer createVertexBuffer(unsigned size) const;
    Buffer createIndexBuffer(unsigned size) const;
    Buffer createBuffer(SDL_GPUBufferUsageFlags usage, unsigned size) const;

    // RGBA8 pixels into a whole texture, in `copy` of a running pass.
    void uploadTexture(SDL_GPUCopyPass *copy, SDL_GPUTexture *texture, unsigned width, unsigned height,
                       std::span<const std::byte> pixels, TransferBuffer &staging) const;

private:
    Device(SDL_GPUDevice *device, const Paths &paths);

    SDL_GPUDevice *m_device;
    const Paths &m_paths;
};

// The SDL_GPU drivers this build has, by SDL's names, in SDL's order of preference.
std::vector<std::string> availableDrivers();

// The last SDL error, with what was being done.
void logGpuError(std::string_view what);

} // namespace alyssapan::gpu
