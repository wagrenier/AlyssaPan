// The render thread: game frames in, pictures out. It owns the GPU's work, ImGui and the present, and
// keeps the window alive on its own: when no new game frame has come it shows the last one again, so the
// overlay, resizing and fullscreen all work through a load, and a vsync stall never holds the EE side. (The
// older siblings render on the game thread; FS2, in YamiPan, flips from a frame-sync thread that must never
// block.)
//
// Carried over from YamiPan with its 2D path only: FS2's model renderer, smooth motion (FrameBlend) and
// whole-frame passes stay there until CT3's own 3D module is written (cpp-port.md "Left in YamiPan").
//
// The picture goes through two targets: the scene, the game's frame at the render resolution, and the
// output, the window's size, where the scene lands in its picture rect with the overlay on top. The output
// keeps one format for the renderer's life, because ImGui bakes it into its pipeline; the present pass
// then writes whatever the swapchain is running.
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include <SDL3/SDL_gpu.h>

#include "alyssapan/core/config.h"
#include "alyssapan/core/profiler.h"
#include "alyssapan/gpu/device.h"
#include "alyssapan/os/thread.h"
#include "alyssapan/rendering/display_geometry.h"
#include "alyssapan/rendering/frame.h"
#include "alyssapan/rendering/frame_pacing.h"
#include "alyssapan/rendering/msaa_targets.h"
#include "alyssapan/rendering/sampler_table.h"
#include "alyssapan/rendering/texture_store.h"
#include "alyssapan/rendering/ui_canvas.h"

namespace alyssapan
{
class EventQueue;
class Window;
}

namespace alyssapan::ui
{
class Overlay;
}

namespace alyssapan::rendering
{

// The GPU's blend for a pipeline.
SDL_GPUColorTargetBlendState blendStateOf(PipelineKey key);
class PresentPass;
class HdrOutput;
class Screenshotter;

class Renderer
{
public:
    Renderer(Window &window, gpu::Device &device, Config &config, EventQueue &events, ui::Overlay &overlay,
             Profiler &profiler);
    ~Renderer();

    void start();
    void stop();

    UiCanvas &canvas() { return m_canvas; }
    Profiler &profiler() { return m_profiler; }
    TextureStore &textures() { return m_textures; }

    // A frame to fill, with its storage from a frame already shown.
    std::unique_ptr<Frame> takeFreeFrame();

    // Hands a finished frame over. Under VSync the frames are shown in order, each for the refreshes it spans
    // (renderOnce); a frame waiting behind kMaxQueuedFrames newer ones is dropped, so the picture never lags
    // further. Other present modes show the newest.
    void submit(std::unique_ptr<Frame> frame);

    // The layout of the last present, from any thread. Read this, never the window, for the shape.
    PictureLayout layout() const;

    struct Stats
    {
        double presentRate = 0.0;
        double gameRate = 0.0;
        std::size_t draws = 0;
        std::size_t vertices = 0;
    };
    Stats stats() const;

    // Applies the settings the swapchain depends on, the present mode and the HDR composition (from the render
    // thread: the overlay, a display change).
    void applyPresentMode();

    const HdrOutput &hdr() const { return *m_hdr; }

    SDL_GPUSampler *samplerFor(bool linear, bool repeat) const;
    // The texture samplers and their anisotropy (render thread: the overlay reads the active level).
    const SamplerTable &samplers() const { return m_samplers; }
    // The scene's multisampling (render thread: the overlay reads the active count).
    const MsaaTargets &msaa() const { return *m_msaa; }
    // How long each game frame stayed on screen (render thread: the overlay's Frame graph).
    const FramePacing &pacing() const { return m_pacing; }

    SDL_GPUTextureFormat outputFormat() const { return m_outputFormat; }

private:
    // Frames waiting to be shown at most: a burst of two presents (a swapchain that releases its images in
    // pairs) finds two.
    static constexpr std::size_t kMaxQueuedFrames = 2;

    void threadMain();
    void renderOnce();
    void ensureTargets(const PictureLayout &layout);
    void uploadImages(SDL_GPUCopyPass *copy, Frame &frame);
    void uploadVertices(SDL_GPUCopyPass *copy, const Frame &frame);
    void recordScene(SDL_GPUCommandBuffer *commands, const Frame &frame, const PictureLayout &layout);
    void recordDraws(SDL_GPUCommandBuffer *commands, const Frame &frame, const PictureLayout &layout,
                     std::size_t first, std::size_t last, bool clear);
    void recordOutput(SDL_GPUCommandBuffer *commands, const PictureLayout &layout);
    SDL_GPUGraphicsPipeline *pipelineFor(PipelineKey key);
    void updateStats(bool newFrame);

    Window &m_window;
    gpu::Device &m_device;
    Config &m_config;
    EventQueue &m_events;
    ui::Overlay &m_overlay;
    Profiler &m_profiler;
    UiCanvas m_canvas;
    TextureStore m_textures;

    SDL_GPUTextureFormat m_sceneFormat;
    SDL_GPUTextureFormat m_outputFormat;
    gpu::Texture m_scene;
    gpu::Texture m_depth; // the scene's, the 3D draws' (the GS's Z buffer)
    SDL_GPUTextureFormat m_depthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    gpu::Texture m_output;
    gpu::Texture m_white;
    int m_sceneWidth = 0;
    int m_sceneHeight = 0;
    int m_outputWidth = 0;
    int m_outputHeight = 0;

    gpu::Shader m_vertexShader;
    gpu::Shader m_fragmentShader;
    std::map<PipelineKey, gpu::GraphicsPipeline> m_pipelines;
    SamplerTable m_samplers;
    std::unique_ptr<MsaaTargets> m_msaa; // after m_depthFormat is chosen
    int m_pipelineSamples = 1;           // the count m_pipelines were built at

    gpu::Buffer m_vertexBuffer;
    gpu::TransferBuffer m_vertexUpload;
    unsigned m_vertexCapacity = 0;
    std::vector<gpu::TransferBuffer> m_staging;
    bool m_whiteUploaded = false;

    std::unique_ptr<HdrOutput> m_hdr;
    std::unique_ptr<PresentPass> m_present;
    std::unique_ptr<Screenshotter> m_screenshots;

    // The frame on screen, kept to draw again when the scene target changes size.
    std::unique_ptr<Frame> m_shown;
    mutable std::mutex m_lock;
    std::deque<std::unique_ptr<Frame>> m_queue; // submitted, not yet shown: oldest first
    std::vector<std::unique_ptr<Frame>> m_free;
    PictureLayout m_layout;
    Stats m_stats;
    FramePacing m_pacing;          // render thread
    unsigned m_shownPresents = 0;  // presents m_shown has had (render thread)

    std::uint64_t m_lastFrameNumber = 0;
    std::uint64_t m_statPresents = 0;
    std::uint64_t m_statFrames = 0;
    std::uint64_t m_statStart = 0;

    std::atomic<bool> m_stopping{false};
    std::unique_ptr<os::Thread> m_thread;
};

} // namespace alyssapan::rendering
