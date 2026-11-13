#include "alyssapan/rendering/renderer.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <thread>

#include <SDL3/SDL_timer.h>

#include "alyssapan/core/event_queue.h"
#include "alyssapan/core/logger.h"
#include "alyssapan/core/window.h"
#include "alyssapan/rendering/hdr_output.h"
#include "alyssapan/rendering/present_pass.h"
#include "alyssapan/rendering/screenshotter.h"
#include "alyssapan/ui/overlay.h"

namespace alyssapan::rendering
{
namespace
{

constexpr std::size_t kRenderThreadStack = 2 * 1024 * 1024;
constexpr unsigned kInitialVertexCapacity = 1024 * 1024;
constexpr auto kIdleDelay = std::chrono::milliseconds(16);

// Mirrors UiVertexUniforms in ui.vert.hlsl.
struct UiVertexUniforms
{
    float frameToClip[4];
    float texelToUv[4];
};

// Mirrors UiFragmentUniforms in ui.frag.hlsl.
struct UiFragmentUniforms
{
    float params[4];
};

SDL_GPUPresentMode sdlPresentMode(PresentMode mode)
{
    switch (mode)
    {
    case PresentMode::Mailbox:
        return SDL_GPU_PRESENTMODE_MAILBOX;
    case PresentMode::Immediate:
        return SDL_GPU_PRESENTMODE_IMMEDIATE;
    case PresentMode::VSync:
        break;
    }
    return SDL_GPU_PRESENTMODE_VSYNC;
}

} // namespace

SDL_GPUColorTargetBlendState blendStateOf(PipelineKey key)
{
    SDL_GPUColorTargetBlendState state{};
    state.enable_blend = key.blend != BlendMode::None;
    state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    // The GS blends colour only: the frame's alpha takes the source's.
    state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    switch (key.blend)
    {
    case BlendMode::None:
    case BlendMode::Alpha:
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    case BlendMode::Additive:
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        break;
    case BlendMode::Subtractive:
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        state.color_blend_op = SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
        break;
    case BlendMode::DestinationAlpha:
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_ALPHA;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
        break;
    case BlendMode::FixedAlpha:
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR;
        break;
    case BlendMode::FixedAdditive:
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        break;
    case BlendMode::DestinationScale:
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
        break;
    case BlendMode::DestinationBoost:
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
        break;
    case BlendMode::DestinationFade:
        state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
        state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
        break;
    }

    switch (key.mask)
    {
    case ColorMask::All:
        break;
    case ColorMask::ColorOnly:
        state.enable_color_write_mask = true;
        state.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G | SDL_GPU_COLORCOMPONENT_B;
        break;
    case ColorMask::AlphaOnly:
        state.enable_color_write_mask = true;
        state.color_write_mask = SDL_GPU_COLORCOMPONENT_A;
        break;
    case ColorMask::None:
        state.enable_color_write_mask = true;
        state.color_write_mask = 0;
        break;
    }
    return state;
}

// The GS's ZTST over a depth of Z / Z max: larger is nearer, as on the GS (cleared to 0).
static SDL_GPUCompareOp depthCompareOf(DepthTest test)
{
    switch (test)
    {
    case DepthTest::Never:
        return SDL_GPU_COMPAREOP_NEVER;
    case DepthTest::GreaterOrEqual:
        return SDL_GPU_COMPAREOP_GREATER_OR_EQUAL;
    case DepthTest::Greater:
        return SDL_GPU_COMPAREOP_GREATER;
    case DepthTest::Off:
    case DepthTest::Always:
        break;
    }
    return SDL_GPU_COMPAREOP_ALWAYS;
}

Renderer::Renderer(Window &window, gpu::Device &device, Config &config, EventQueue &events, ui::Overlay &overlay,
                   Profiler &profiler)
    : m_window(window), m_device(device), m_config(config), m_events(events), m_overlay(overlay),
      m_profiler(profiler), m_canvas(*this),
      m_sceneFormat(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM), m_outputFormat(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM),
      m_samplers(device)
{
    m_vertexShader = m_device.loadShader("ui.vert", SDL_GPU_SHADERSTAGE_VERTEX, {.samplers = 0, .uniformBuffers = 1});
    m_fragmentShader =
        m_device.loadShader("ui.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, {.samplers = 1, .uniformBuffers = 1});

    m_samplers.update(m_config.display.anisotropy);

    // A float depth over the GS's 1 / z-shaped Z keeps its precision where the GS has it (D32 is everywhere
    // SDL_GPU runs; D24 the fallback).
    m_depthFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    if (!SDL_GPUTextureSupportsFormat(m_device.get(), m_depthFormat, SDL_GPU_TEXTURETYPE_2D,
                                      SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
        m_depthFormat = SDL_GPU_TEXTUREFORMAT_D24_UNORM;
    m_msaa = std::make_unique<MsaaTargets>(m_device, m_sceneFormat, m_depthFormat);
    m_msaa->update(m_config.display.msaa);

    m_white = m_device.createTexture(1, 1, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, SDL_GPU_TEXTUREUSAGE_SAMPLER);
    m_hdr = std::make_unique<HdrOutput>(m_device, m_window, m_config);
    m_present = std::make_unique<PresentPass>(m_device, m_window, m_config, *m_hdr);
    m_screenshots = std::make_unique<Screenshotter>(m_device);
    applyPresentMode();
}

Renderer::~Renderer()
{
    stop();
}

void Renderer::start()
{
    m_statStart = SDL_GetTicks();
    m_thread = std::make_unique<os::Thread>("AlyssaPanRender", kRenderThreadStack, [this] { threadMain(); });
}

void Renderer::stop()
{
    m_stopping.store(true);
    m_thread.reset();
}

std::unique_ptr<Frame> Renderer::takeFreeFrame()
{
    std::scoped_lock lock(m_lock);
    if (m_free.empty())
        return std::make_unique<Frame>();
    std::unique_ptr<Frame> frame = std::move(m_free.back());
    m_free.pop_back();
    frame->clear();
    return frame;
}

void Renderer::submit(std::unique_ptr<Frame> frame)
{
    std::scoped_lock lock(m_lock);
    if (m_incoming != nullptr)
        m_free.push_back(std::move(m_incoming)); // never shown: overtaken
    m_incoming = std::move(frame);
}

PictureLayout Renderer::layout() const
{
    std::scoped_lock lock(m_lock);
    return m_layout;
}

Renderer::Stats Renderer::stats() const
{
    std::scoped_lock lock(m_lock);
    return m_stats;
}

void Renderer::applyPresentMode()
{
    SDL_GPUPresentMode mode = sdlPresentMode(m_config.display.presentMode);
    if (!SDL_WindowSupportsGPUPresentMode(m_device.get(), m_window.get(), mode))
        mode = SDL_GPU_PRESENTMODE_VSYNC;
    m_hdr->apply(mode);
}

void Renderer::threadMain()
{
    m_overlay.initializeOnRenderThread(m_outputFormat);
    while (!m_stopping.load())
        renderOnce();
    SDL_WaitForGPUIdle(m_device.get());
    m_overlay.shutdownOnRenderThread();
}

void Renderer::ensureTargets(const PictureLayout &layout)
{
    constexpr SDL_GPUTextureUsageFlags kUsage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;

    if (!m_scene || m_sceneWidth != layout.renderWidth || m_sceneHeight != layout.renderHeight)
    {
        m_scene = m_device.createTexture(static_cast<unsigned>(layout.renderWidth),
                                         static_cast<unsigned>(layout.renderHeight), m_sceneFormat, kUsage);
        m_depth = m_device.createTexture(static_cast<unsigned>(layout.renderWidth),
                                         static_cast<unsigned>(layout.renderHeight), m_depthFormat,
                                         SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);
        m_sceneWidth = layout.renderWidth;
        m_sceneHeight = layout.renderHeight;
    }
    m_msaa->ensure(layout.renderWidth, layout.renderHeight);
    if (!m_output || m_outputWidth != layout.windowWidth || m_outputHeight != layout.windowHeight)
    {
        m_output = m_device.createTexture(static_cast<unsigned>(layout.windowWidth),
                                          static_cast<unsigned>(layout.windowHeight), m_outputFormat, kUsage);
        m_outputWidth = layout.windowWidth;
        m_outputHeight = layout.windowHeight;
    }
}

SDL_GPUGraphicsPipeline *Renderer::pipelineFor(PipelineKey key)
{
    if (const auto found = m_pipelines.find(key); found != m_pipelines.end())
        return found->second.get();

    SDL_GPUVertexBufferDescription buffer{};
    buffer.slot = 0;
    buffer.pitch = sizeof(Vertex);
    buffer.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    const std::array<SDL_GPUVertexAttribute, 3> attributes{{
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Vertex, x)},
        {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Vertex, u)},
        {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, offsetof(Vertex, color)},
    }};

    SDL_GPUColorTargetDescription target{};
    target.format = m_sceneFormat;
    target.blend_state = blendStateOf(key);

    SDL_GPUGraphicsPipelineCreateInfo info{};
    // Every scene pass has the depth target bound; the 2D draws (DepthTest::Off) leave it alone.
    info.target_info.has_depth_stencil_target = true;
    info.target_info.depth_stencil_format = m_depthFormat;
    info.depth_stencil_state.enable_depth_test = key.depth != DepthTest::Off;
    info.depth_stencil_state.enable_depth_write = key.depth != DepthTest::Off && key.depthWrite;
    info.depth_stencil_state.compare_op = depthCompareOf(key.depth);
    info.vertex_shader = m_vertexShader.get();
    info.fragment_shader = m_fragmentShader.get();
    info.vertex_input_state.vertex_buffer_descriptions = &buffer;
    info.vertex_input_state.num_vertex_buffers = 1;
    info.vertex_input_state.vertex_attributes = attributes.data();
    info.vertex_input_state.num_vertex_attributes = attributes.size();
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    // At the scene's count: a pipeline bakes it, so a change of count empties the cache (renderOnce).
    info.multisample_state.sample_count = m_msaa->sampleCount();
    info.target_info.color_target_descriptions = &target;
    info.target_info.num_color_targets = 1;

    SDL_GPUGraphicsPipeline *const pipeline = SDL_CreateGPUGraphicsPipeline(m_device.get(), &info);
    if (pipeline == nullptr)
        gpu::logGpuError("SDL_CreateGPUGraphicsPipeline");
    m_pipelines.emplace(key, gpu::GraphicsPipeline(m_device.get(), pipeline));
    return pipeline;
}

SDL_GPUSampler *Renderer::samplerFor(bool linear, bool repeat) const
{
    return m_samplers.get(linear, repeat);
}

void Renderer::uploadImages(SDL_GPUCopyPass *copy, Frame &frame)
{
    const auto upload = [&](Image *image) {
        if (image == nullptr || image->texture != nullptr || image->pixels.empty())
            return;

        image->texture = std::make_shared<gpu::Texture>(
            m_device.createTexture(static_cast<unsigned>(image->width), static_cast<unsigned>(image->height),
                                   SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, SDL_GPU_TEXTUREUSAGE_SAMPLER));
        m_device.uploadTexture(copy, image->texture->get(), static_cast<unsigned>(image->width),
                               static_cast<unsigned>(image->height), image->pixels,
                               m_staging.emplace_back());
        std::vector<std::byte>().swap(image->pixels);
    };

    for (const DrawCall &draw : frame.draws)
        upload(draw.image.get());
}

void Renderer::uploadVertices(SDL_GPUCopyPass *copy, const Frame &frame)
{
    const auto bytes = static_cast<unsigned>(frame.vertices.size() * sizeof(Vertex));
    if (bytes == 0)
        return;

    if (bytes > m_vertexCapacity || !m_vertexBuffer)
    {
        m_vertexCapacity = std::max({bytes * 2, m_vertexCapacity, kInitialVertexCapacity});
        m_vertexBuffer = m_device.createVertexBuffer(m_vertexCapacity);
        m_vertexUpload = m_device.createUploadBuffer(m_vertexCapacity);
    }

    void *const mapped = SDL_MapGPUTransferBuffer(m_device.get(), m_vertexUpload.get(), true);
    if (mapped == nullptr)
        return;
    std::memcpy(mapped, frame.vertices.data(), bytes);
    SDL_UnmapGPUTransferBuffer(m_device.get(), m_vertexUpload.get());

    SDL_GPUTransferBufferLocation source{};
    source.transfer_buffer = m_vertexUpload.get();

    SDL_GPUBufferRegion destination{};
    destination.buffer = m_vertexBuffer.get();
    destination.size = bytes;

    // Not cycled (2026-11-10): with cycle on, the scene pass drew nothing from this buffer on D3D12 and
    // Vulkan alike (a probe quad stayed invisible). The transfer buffer's map above still cycles, so the CPU
    // never writes data a frame in flight is copying; the copy itself is ordered on the one queue.
    SDL_UploadToGPUBuffer(copy, &source, &destination, false);
}

void Renderer::recordDraws(SDL_GPUCommandBuffer *commands, const Frame &frame, const PictureLayout &layout,
                           std::size_t first, std::size_t last, bool clear)
{
    if (first == last && !clear)
        return;

    // With MSAA the pass draws into the multisampled target and resolves into the scene texture; the samples are
    // kept (RESOLVE_AND_STORE) so a later pass over the same frame can load them, as a split scene will.
    const bool multisampled = m_msaa->active() && m_msaa->color() != nullptr;
    SDL_GPUColorTargetInfo target{};
    target.texture = multisampled ? m_msaa->color() : m_scene.get();
    target.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    target.store_op = multisampled ? SDL_GPU_STOREOP_RESOLVE_AND_STORE : SDL_GPU_STOREOP_STORE;
    target.resolve_texture = multisampled ? m_scene.get() : nullptr;
    target.clear_color = {0.0f, 0.0f, 0.0f, 0.0f};

    // Cleared with the colour, to 0: the GS's farthest.
    SDL_GPUDepthStencilTargetInfo depth{};
    depth.texture = multisampled ? m_msaa->depth() : m_depth.get();
    depth.clear_depth = 0.0f;
    depth.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
    depth.store_op = SDL_GPU_STOREOP_STORE;
    depth.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depth.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass *const pass = SDL_BeginGPURenderPass(commands, &target, 1, &depth);
    if (pass == nullptr)
    {
        gpu::logGpuError("SDL_BeginGPURenderPass(scene)");
        return;
    }

    if (first != last)
    {
        SDL_GPUBufferBinding vertices{};
        vertices.buffer = m_vertexBuffer.get();
        SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);
    }

    // The frame fills the scene target, whatever its shape.
    const float scaleX = static_cast<float>(layout.renderWidth) / kFrameWidth;
    const float scaleY = static_cast<float>(layout.renderHeight) / kFrameHeight;

    for (std::size_t index = first; index < last; index++)
    {
        const DrawCall &draw = frame.draws[index];
        SDL_GPUGraphicsPipeline *const pipeline = pipelineFor(draw.pipeline);
        if (pipeline == nullptr || draw.vertexCount == 0)
            continue;
        SDL_BindGPUGraphicsPipeline(pass, pipeline);

        const Image *const image = draw.image.get();
        SDL_GPUTexture *const texture = image != nullptr && image->texture != nullptr ? image->texture->get()
                                                                                       : m_white.get();
        SDL_GPUTextureSamplerBinding binding{};
        binding.texture = texture;
        binding.sampler = samplerFor(draw.linearFilter, draw.repeat);
        SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);

        const float texelWidth = image != nullptr ? 1.0f / static_cast<float>(image->width) : 1.0f;
        const float texelHeight = image != nullptr ? 1.0f / static_cast<float>(image->height) : 1.0f;
        const UiVertexUniforms vertexUniforms{{2.0f / kFrameWidth, -2.0f / kFrameHeight, -1.0f, 1.0f},
                                              {texelWidth, texelHeight, 0.0f, 0.0f}};
        SDL_PushGPUVertexUniformData(commands, 0, &vertexUniforms, sizeof(vertexUniforms));

        // A texture modulates on the GS's scale, 0x80 as 1.0; a flat colour goes out as it is.
        const float colourScale = image != nullptr ? 255.0f / 128.0f : 1.0f;
        const UiFragmentUniforms fragmentUniforms{{static_cast<float>(draw.alphaTestRef) / 128.0f, colourScale,
                                                   draw.alphaTestInverted ? 1.0f : 0.0f, 0.0f}};
        SDL_PushGPUFragmentUniformData(commands, 0, &fragmentUniforms, sizeof(fragmentUniforms));

        const float fixed = static_cast<float>(draw.fixedAlpha) / 128.0f;
        const SDL_FColor constants{fixed, fixed, fixed, fixed};
        SDL_SetGPUBlendConstants(pass, constants);

        const auto clampX = [&](float x) { return std::clamp(static_cast<int>(x * scaleX), 0, layout.renderWidth); };
        const auto clampY = [&](float y) { return std::clamp(static_cast<int>(y * scaleY), 0, layout.renderHeight); };
        const SDL_Rect scissor{clampX(draw.scissor.x0), clampY(draw.scissor.y0),
                               clampX(draw.scissor.x1) - clampX(draw.scissor.x0),
                               clampY(draw.scissor.y1) - clampY(draw.scissor.y0)};
        SDL_SetGPUScissor(pass, &scissor);

        SDL_DrawGPUPrimitives(pass, draw.vertexCount, 1, draw.firstVertex, 0);
    }
    SDL_EndGPURenderPass(pass);
}

void Renderer::recordScene(SDL_GPUCommandBuffer *commands, const Frame &frame, const PictureLayout &layout)
{
    // One pass over the scene target for now. A 3D module (a depth target) or a whole-frame pass (one that
    // reads the scene) splits it, as in YamiPan's renderer.
    recordDraws(commands, frame, layout, 0, frame.draws.size(), true);
}

void Renderer::recordOutput(SDL_GPUCommandBuffer *commands, const PictureLayout &layout)
{
    // The picture in its rect, black around it: the bars of a shape that is not the window's.
    SDL_GPUBlitInfo blit{};
    blit.source.texture = m_scene.get();
    blit.source.w = static_cast<Uint32>(layout.renderWidth);
    blit.source.h = static_cast<Uint32>(layout.renderHeight);
    blit.destination.texture = m_output.get();
    blit.destination.x = static_cast<Uint32>(layout.picture.x);
    blit.destination.y = static_cast<Uint32>(layout.picture.y);
    blit.destination.w = static_cast<Uint32>(layout.picture.width);
    blit.destination.h = static_cast<Uint32>(layout.picture.height);
    blit.load_op = SDL_GPU_LOADOP_CLEAR;
    blit.clear_color = {0.0f, 0.0f, 0.0f, 1.0f};
    blit.filter = m_config.display.upscaleFilter == UpscaleFilter::Linear ? SDL_GPU_FILTER_LINEAR
                                                                           : SDL_GPU_FILTER_NEAREST;
    SDL_BlitGPUTexture(commands, &blit);
}

void Renderer::updateStats(bool newFrame)
{
    m_statPresents++;
    if (newFrame)
        m_statFrames++;

    const std::uint64_t now = SDL_GetTicks();
    const std::uint64_t elapsed = now - m_statStart;
    if (elapsed < 500)
        return;

    std::scoped_lock lock(m_lock);
    m_stats.presentRate = static_cast<double>(m_statPresents) * 1000.0 / static_cast<double>(elapsed);
    m_stats.gameRate = static_cast<double>(m_statFrames) * 1000.0 / static_cast<double>(elapsed);
    if (m_shown != nullptr)
    {
        m_stats.draws = m_shown->draws.size();
        m_stats.vertices = m_shown->vertices.size();
    }
    m_statPresents = 0;
    m_statFrames = 0;
    m_statStart = now;
}

void Renderer::renderOnce()
{
    const auto [windowWidth, windowHeight] = m_window.pixelSize();
    if (windowWidth <= 0 || windowHeight <= 0)
    {
        std::this_thread::sleep_for(kIdleDelay);
        return;
    }

    bool displayChanged = false;
    m_events.drain([this, &displayChanged](const SDL_Event &event) {
        // The window moved to another display, or HDR was switched in the OS: Auto follows it, and an automatic
        // paper white follows the OS's SDR level.
        if (event.type == SDL_EVENT_WINDOW_HDR_STATE_CHANGED || event.type == SDL_EVENT_WINDOW_DISPLAY_CHANGED)
            displayChanged = true;
        m_overlay.processEvent(event);
    });
    if (displayChanged)
    {
        m_hdr->refreshDisplay();
        applyPresentMode();
    }
    // Between frames: the overlay's Graphics menu may have moved the anisotropy.
    m_samplers.update(m_config.display.anisotropy);
    // ... and the MSAA count.
    m_msaa->update(m_config.display.msaa);

    const PictureLayout layout = computeLayout(windowWidth, windowHeight, m_config.display);
    const bool layoutChanged = [&] {
        std::scoped_lock lock(m_lock);
        const bool changed = !(m_layout == layout);
        m_layout = layout;
        return changed;
    }();
    ensureTargets(layout);
    // A new count (the menu, or targets the device refused at this size): pipelines bake it, and the shown frame
    // is drawn again at it.
    const bool samplesChanged = m_msaa->activeCount() != m_pipelineSamples;
    if (samplesChanged)
    {
        m_pipelines.clear();
        m_pipelineSamples = m_msaa->activeCount();
    }

    bool newFrame = false;
    {
        std::scoped_lock lock(m_lock);
        if (m_incoming != nullptr)
        {
            if (m_shown != nullptr)
                m_free.push_back(std::move(m_shown));
            m_shown = std::move(m_incoming);
            newFrame = true;
        }
    }
    const bool redraw = newFrame || layoutChanged || samplesChanged;

    {
        Profiler::Scope phase(m_profiler, Profiler::Phase::Overlay);
        m_overlay.beginFrame();
        m_overlay.draw(*this);
        m_overlay.endFrame();
    }

    SDL_GPUCommandBuffer *const commands = SDL_AcquireGPUCommandBuffer(m_device.get());
    if (commands == nullptr)
    {
        gpu::logGpuError("SDL_AcquireGPUCommandBuffer");
        return;
    }

    std::optional<Profiler::Scope> phase(std::in_place, m_profiler, Profiler::Phase::Upload);
    SDL_GPUCopyPass *const copy = SDL_BeginGPUCopyPass(commands);
    if (!m_whiteUploaded)
    {
        constexpr std::array<std::byte, 4> kWhitePixel{std::byte{0xff}, std::byte{0xff}, std::byte{0xff},
                                                       std::byte{0xff}};
        m_device.uploadTexture(copy, m_white.get(), 1, 1, kWhitePixel, m_staging.emplace_back());
        m_whiteUploaded = true;
    }
    if (redraw && m_shown != nullptr)
    {
        uploadImages(copy, *m_shown);
        uploadVertices(copy, *m_shown);
    }
    SDL_EndGPUCopyPass(copy);

    phase.emplace(m_profiler, Profiler::Phase::Record);
    static const Frame kEmptyFrame;
    if (redraw || m_shown == nullptr)
        recordScene(commands, m_shown != nullptr ? *m_shown : kEmptyFrame, layout);
    recordOutput(commands, layout);

    m_overlay.prepareDrawData(commands);
    {
        SDL_GPUColorTargetInfo target{};
        target.texture = m_output.get();
        target.load_op = SDL_GPU_LOADOP_LOAD;
        target.store_op = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass *const pass = SDL_BeginGPURenderPass(commands, &target, 1, nullptr);
        if (pass != nullptr)
        {
            m_overlay.renderDrawData(commands, pass);
            SDL_EndGPURenderPass(pass);
        }
    }

    const bool shotDue = m_screenshots->recordIfDue(commands, m_scene.get(), m_output.get(), layout);

    SDL_GPUTexture *swapchain = nullptr;
    Uint32 swapchainWidth = 0;
    Uint32 swapchainHeight = 0;
    phase.emplace(m_profiler, Profiler::Phase::SwapchainAcquire);
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, m_window.get(), &swapchain, &swapchainWidth, &swapchainHeight))
    {
        gpu::logGpuError("SDL_AcquireGPUSwapchainTexture");
        SDL_CancelGPUCommandBuffer(commands);
        return;
    }
    phase.emplace(m_profiler, Profiler::Phase::Record);
    if (swapchain != nullptr)
        m_present->record(commands, m_output.get(), swapchain, swapchainWidth, swapchainHeight);

    phase.emplace(m_profiler, Profiler::Phase::Submit);
    if (!SDL_SubmitGPUCommandBuffer(commands))
        gpu::logGpuError("SDL_SubmitGPUCommandBuffer");
    m_staging.clear();
    phase.reset();

    if (shotDue)
        m_screenshots->write();

    if (swapchain == nullptr)
        std::this_thread::sleep_for(kIdleDelay); // minimised: nothing paces this loop
    updateStats(newFrame);

    // A slow present says what it carried: a new game frame (its upload) or a repeat, and how much.
    if (m_profiler.stutterThresholdMs() > 0.0)
    {
        const std::string note =
            m_shown == nullptr ? std::string("no frame")
                               : std::format("{} {}: {} draws, {} vertices{}", newFrame ? "frame" : "repeat of",
                                             m_shown->number, m_shown->draws.size(), m_shown->vertices.size(),
                                             shotDue ? ", screenshot" : "");
        m_profiler.endFrame(Profiler::Lane::Render, note);
    }
    else
    {
        m_profiler.endFrame(Profiler::Lane::Render);
    }
}

} // namespace alyssapan::rendering
