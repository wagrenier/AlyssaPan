// One game frame's draws, as the render thread takes them: plain data with no pointer back into game
// memory, so the game can go on while the GPU works, and the last frame can be shown again.
#pragma once

#include <array>
#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "alyssapan/gpu/device.h"

namespace alyssapan::rendering
{

// How GS blending maps to the host's: ALPHA_1's (A - B) * C + D, by what the game actually uses.
enum class BlendMode : std::uint8_t
{
    None,
    Alpha,          // (Cs - Cd) * As + Cd
    Additive,       // Cs * As + Cd
    Subtractive,    // Cd - Cs * As
    DestinationAlpha, // (Cs - Cd) * Ad + Cd
    FixedAlpha,     // (Cs - Cd) * FIX + Cd
    FixedAdditive,  // Cs * FIX + Cd
    DestinationScale, // Cd * FIX, FIX up to 1
    DestinationBoost, // Cs * Cd + Cd; ALPHA_1 0x49, Cd * As + Cd, when the fragment hands As over as Cs
    DestinationFade, // (0 - Cd) * As + Cd
};

// ALPHA_1's selectors as the blend above.
BlendMode blendModeOf(std::uint64_t alpha);

// FRAME_1's FBMSK as the two halves the game's presets use, or all of it (a depth-only pass: TEST_1's
// AFAIL ZB_ONLY under ATST NEVER, or FBMSK 0xffffffff).
enum class ColorMask : std::uint8_t
{
    All,
    ColorOnly,
    AlphaOnly,
    None,
};

// TEX0's TFX (with TCC where it decides the alpha), as ui.frag combines the texel Ct with the vertex colour
// Cf. DECAL needs none: its callers send the vertex colour as 0x80, which MODULATE turns into the texel.
// The values are ui.frag's (uParams.w).
enum class TextureFunction : std::uint8_t
{
    Modulate = 0,           // Cv = Ct Cf, Av = At Af (TCC 1) or Af
    Highlight = 1,          // Cv = Ct Cf + Af, Av = At + Af (TCC 1)
    Highlight2 = 2,         // Cv = Ct Cf + Af, Av = At (TCC 1)
    HighlightVertexAlpha = 3, // HIGHLIGHT or HIGHLIGHT2 under TCC 0: Cv = Ct Cf + Af, Av = Af
};

// TEST_1's ZTST, the GS's order: a larger Z is nearer. Off: the 2D draws, which neither test nor write depth.
enum class DepthTest : std::uint8_t
{
    Off,
    Never,
    Always,
    GreaterOrEqual,
    Greater,
};

// The stencil's part in CT3's shadow pass (alyssapan_drawga.h AlyssaPan_DrawGaShadow*). The ROM counts shadow-
// volume faces into a GS frame cleared to 0x80 and lays the folded count over the display frame; the host
// keeps the count in the stencil buffer instead, from the same 0x80 (kShadowStencilBase). Off: every other draw.
enum class StencilMode : std::uint8_t
{
    Off,
    ShadowReset,     // the count back to kShadowStencilBase (asShadowStart's clear sprite)
    ShadowIncrement, // a minus face (ALPHA_1 Cs * As + Cd): +1 where it passes the depth test
    ShadowDecrement, // a plus face (Cd - Cs * As): -1
    ShadowTest,      // drawn only where the count is above kShadowStencilBase (the overlay)
};
constexpr std::uint8_t kShadowStencilBase = 0x80;

struct PipelineKey
{
    BlendMode blend = BlendMode::None;
    ColorMask mask = ColorMask::All;
    DepthTest depth = DepthTest::Off;
    bool depthWrite = false; // ZBUF_1's ZMSK clear
    StencilMode stencil = StencilMode::Off;

    auto operator<=>(const PipelineKey &) const = default;
};

// An image decoded at its source, shared by every frame that draws it, so it outlives the game's own
// texture object for as long as a frame on screen needs it.
struct Image
{
    int width = 0;
    int height = 0;
    std::vector<std::byte> pixels; // RGBA8
    std::shared_ptr<gpu::Texture> texture;
};

// In frame units; vertices in texels.
struct Vertex
{
    float x, y;
    float z; // the GS's Z over the Z buffer's largest, 0..1 (1 nearest); 0 for what tests no depth
    float u, v;
    std::uint32_t color; // the GS's RGBAQ bytes: 0x80 is 1.0
};
static_assert(sizeof(Vertex) == 24);

struct Scissor
{
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 512.0f;
    float y1 = 512.0f;

    bool operator==(const Scissor &) const = default;
};

struct DrawCall
{
    PipelineKey pipeline;
    std::shared_ptr<Image> image; // null: untextured
    Scissor scissor;
    bool linearFilter = false;
    bool repeat = false;
    std::uint8_t alphaTestRef = 0; // fragments below it are dropped; 0 keeps them all
    bool alphaTestInverted = false; // the fragments at or over it are dropped instead (AFAIL's second pass)
    std::uint8_t fixedAlpha = 0x80;
    TextureFunction textureFunction = TextureFunction::Modulate;
    std::uint32_t firstVertex = 0;
    std::uint32_t vertexCount = 0;
};

struct Frame
{
    std::vector<Vertex> vertices;
    std::vector<DrawCall> draws;
    std::uint64_t number = 0;
    // When the field that released this frame was due (the interrupt controller's clock): what smooth motion
    // will blend two frames by, once a 3D module brings it back (cpp-port.md "Left in YamiPan").
    std::chrono::steady_clock::time_point time{};

    void clear()
    {
        vertices.clear();
        draws.clear();
    }
};

} // namespace alyssapan::rendering
