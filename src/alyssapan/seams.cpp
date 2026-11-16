// The seams game code calls (cpp-port.md "Scope"): thin forwarders, so the game's side never changes.
// Carried over from YamiPan without FS2's 3D, shadow and whole-frame effect seams (cpp-port.md "Left in
// YamiPan"): CT3's own draw modules add theirs here when their TUs are read.
#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>

#include "alyssapan/alyssapan_memory.h"
#include "alyssapan/core/application.h"
#include "alyssapan/ee/interrupts.h"
#include "alyssapan/ee/kernel.h"
#include "alyssapan/os/alyssapan_time.h"
#include "alyssapan/os/timed_waiter.h"
#include "alyssapan/rendering/alyssapan_drawga.h"
#include "alyssapan/rendering/alyssapan_drawui.h"
#include "alyssapan/rendering/ga_mesh.h"
#include "alyssapan/rendering/frame.h"
#include "alyssapan/rendering/renderer.h"
#include "alyssapan/rendering/texture_store.h"
#include "alyssapan/rendering/ui_canvas.h"

namespace
{

alyssapan::rendering::UiCanvas &canvas()
{
    return alyssapan::Application::instance().renderer().canvas();
}

} // namespace

// Not std::this_thread::sleep_for: under MinGW a 1 ms one takes 15.6 ms (os/timed_waiter.h).
void AlyssaPan_Sleep(unsigned int milliseconds)
{
    alyssapan::os::TimedWaiter::sleepFor(std::chrono::milliseconds(milliseconds));
}

void AlyssaPan_SleepNs(uint64_t nanoseconds)
{
    alyssapan::os::TimedWaiter::sleepFor(std::chrono::nanoseconds(nanoseconds));
}

namespace
{
std::chrono::steady_clock::time_point s_fieldWaitStart;
}

void AlyssaPan_FieldWaitBegin(void)
{
    s_fieldWaitStart = std::chrono::steady_clock::now();
}

void AlyssaPan_FieldWaitEnd(void)
{
    alyssapan::Application::instance().profiler().addPhase(alyssapan::Profiler::Phase::FieldWait,
                                                           std::chrono::steady_clock::now() - s_fieldWaitStart);
}

void *AlyssaPan_GetHostPointer(uintptr_t)
{
    return nullptr; // no emulated EE memory
}

void AlyssaPan_DrawUiReg(const void *list, int reg, uint64_t value)
{
    canvas().setRegister(list, reg, value);
}

void AlyssaPan_DrawUiPrim(const void *list, uint64_t prim, const uint8_t *rgbaq)
{
    canvas().setPrimitive(list, prim, rgbaq);
}

void AlyssaPan_DrawUiTexture(const void *list, const void *tim2, int picture)
{
    if (tim2 == nullptr)
        return; // the bound one stays

    alyssapan::rendering::Renderer &renderer = alyssapan::Application::instance().renderer();
    std::shared_ptr<alyssapan::rendering::Image> image;
    {
        alyssapan::Profiler::Scope phase(renderer.profiler(), alyssapan::Profiler::Phase::Textures);
        image = renderer.textures().imageOfTim2(tim2, picture);
    }
    renderer.canvas().bindImage(list, std::move(image));
}

void AlyssaPan_DrawUiSqueeze(const void *list, int on)
{
    canvas().setSqueezed(list, on != 0);
}

float AlyssaPan_DrawUiSqueezeFactor(void)
{
    return canvas().squeezeFactor();
}

void AlyssaPan_DrawUiFit(const void *list, int on)
{
    canvas().setFitted(list, on != 0);
}

void AlyssaPan_DrawUiFrameScale(const void *list, float factor)
{
    canvas().scaleFrame(list, factor);
}

void AlyssaPan_DrawUiFrameContrast(const void *list, uint32_t rgba)
{
    canvas().contrastFrame(list, rgba);
}

void AlyssaPan_DrawUiBegin(const void *list, int format)
{
    canvas().begin(list, format);
}

void AlyssaPan_DrawUiVertex(const void *list, const AlyssaPanUiVertex *vertex)
{
    canvas().addVertex(list, *vertex);
}

void AlyssaPan_DrawUiMovie(const void *list, const void *owner, int picture, const void *macroblocks, int columns,
                           int rows, int x, int y, int width, int height)
{
    canvas().drawMacroblocks(list, owner, picture, macroblocks, columns, rows, static_cast<float>(x),
                             static_cast<float>(y), static_cast<float>(width), static_cast<float>(height));
}

namespace
{

// CT3's draw environment as the VU1 program's projection sees it (architecture.md "The VU1 object program":
// vf17's screen centre): a 512 x 448 frame, window 2048..2560 x 2048..2496, centred on (2304, 2272).
// UiCanvas's frame is 512 x 512 centred on its window's (2048, 2048): CT3's frame goes to it centred, its 448
// lines stretched to the canvas's 512, as the TV shows the 448 lines as the full picture.
constexpr float kGaWindowUnit = 16.0f;        // 12.4
constexpr float kGaCentreX = 2304.0f;
constexpr float kGaCentreY = 2272.0f;
constexpr float kGaFrameHeight = 448.0f;
constexpr float kCanvasCentre = 2048.0f;
constexpr float kCanvasFrameHeight = 512.0f;
constexpr float kWindowMax = 65535.0f;
constexpr float kTexelMax = 65535.0f;

// The GS registers UiCanvas::setRegister reads, and the values the ga path's draws take.
constexpr int kRegTex01 = 0x06;
constexpr int kRegClamp1 = 0x08;
constexpr int kRegScissor1 = 0x40;
constexpr int kScissorX1Shift = 16; // SCISSOR_1: SCAX0 0, SCAX1 16, SCAY0 32, SCAY1 48
constexpr int kScissorY1Shift = 48;
constexpr float kCanvasFrameWidth = 512.0f;
constexpr int kRegTex11 = 0x14;
constexpr int kRegAlpha1 = 0x42;
constexpr int kRegTest1 = 0x47;
constexpr int kRegFrame1 = 0x4c;
// Before the game's first draw control (GaMeshRecorder::GsState::controlled): (Cs - Cd) x As + Cd. PORT:
// TEX1_1 is not read: bilinear magnification and minification.
constexpr std::uint64_t kGaAlphaBlend = 0x44;
constexpr std::uint64_t kGaTex1Linear = 1ull << 5 | 1ull << 6;

// PRIM (and PRMODE, the same bits): triangles, Gouraud, textured, blended.
constexpr std::uint64_t kPrimTriangle = 3;
constexpr std::uint64_t kPrimGouraud = 1 << 3;
constexpr std::uint64_t kPrimTextured = 1 << 4;
constexpr std::uint64_t kPrimBlended = 1 << 6;

// TEST_1's fields.
constexpr std::uint64_t kTestAte = 1;
constexpr int kTestAtstShift = 1;
constexpr int kTestAfailShift = 12;
constexpr std::uint64_t kTestZte = 1ull << 16;
constexpr int kTestZtstShift = 17;
enum GaAtst
{
    GaAtst_Never = 0,
    GaAtst_Always = 1,
    GaAtst_GreaterOrEqual = 5,
    GaAtst_Greater = 6,
};
enum GaAfail
{
    GaAfail_Keep = 0,
    GaAfail_FrameOnly = 1,
    GaAfail_DepthOnly = 2,
    GaAfail_RgbOnly = 3,
};
enum GaZtst
{
    GaZtst_Never = 0,
    GaZtst_Always = 1,
    GaZtst_GreaterOrEqual = 2,
    GaZtst_Greater = 3,
};

// ZBUF_1: PSM (its low 4 bits, at 24) and ZMSK (32). FRAME_1's FBMSK alpha byte (RGB_ONLY keeps the alpha).
constexpr int kZbufPsmShift = 24;
constexpr std::uint64_t kZbufPsmMask = 0xf;
constexpr std::uint64_t kZbufZmsk = 1ull << 32;
constexpr std::uint64_t kFrameFbmskAlpha = 0xff000000ull << 32;
enum GaZpsm
{
    GaZpsm_Z32 = 0,
    GaZpsm_Z24 = 1,
};
// The Z buffer's largest value by its format (gaInit3dlib.h GA_ZMAX_*).
constexpr float kZMaxZ32 = 4294967040.0f;
constexpr float kZMaxZ24 = 16777215.0f;
constexpr float kZMaxZ16 = 65535.0f;

// How a mesh draws, from the GS state it came under: TEST_1's alpha test and AFAIL, its depth test, ZBUF_1's
// mask and format, PRMODE's attributes and ALPHA_1, picked apart into the canvas's draw state (no GS
// emulation: per-fragment results the host has no equivalent for are named below).
struct GaDraw
{
    bool skip = false;
    bool textured = false;
    bool gouraud = true;
    bool blended = false;
    std::uint64_t alpha = kGaAlphaBlend;
    std::uint64_t test = 0;  // what the canvas's alpha test reads: a discard under AREF for GEQUAL / GREATER
    std::uint64_t frame = 0; // FBMSK
    alyssapan::rendering::DepthTest depth = alyssapan::rendering::DepthTest::Off;
    bool depthWrite = false;
    bool depthOnly = false;
    bool zWritable = false; // ZBUF_1's ZMSK clear
    float zMax = kZMaxZ32;
    // What the fragments that fail the alpha test still write (GaAfail): a second pass of the mesh with the
    // test inverted. GaAfail_Keep: none.
    int failPass = GaAfail_Keep;
};

GaDraw gaDrawOf(const alyssapan::rendering::GaMeshRecorder::GsState &gs, bool hasImage)
{
    using alyssapan::rendering::DepthTest;
    GaDraw d;
    if (!gs.controlled) {
        // No draw control yet: as these meshes drew before the state was read.
        d.textured = hasImage;
        d.blended = hasImage;
        return d;
    }
    // A TME draw with no picture decoded draws untextured (the white sheet beats nothing for a model).
    d.textured = (gs.prmode & kPrimTextured) != 0 && hasImage;
    d.gouraud = (gs.prmode & kPrimGouraud) != 0;
    d.blended = (gs.prmode & kPrimBlended) != 0;
    d.alpha = gs.alpha;
    d.frame = gs.frame;

    bool colourOnly = false;
    if ((gs.test & kTestAte) != 0) {
        const auto atst = static_cast<int>(gs.test >> kTestAtstShift & 7);
        const auto afail = static_cast<int>(gs.test >> kTestAfailShift & 3);
        if (atst == GaAtst_Never) {
            switch (afail) {
            case GaAfail_Keep:
                d.skip = true;
                return d;
            case GaAfail_FrameOnly:
                colourOnly = true;
                break;
            case GaAfail_DepthOnly:
                d.depthOnly = true; // the depth pre-pass (OneMaxObjectDrawZOnly): depth and no colour
                break;
            case GaAfail_RgbOnly:
                colourOnly = true;
                d.frame |= kFrameFbmskAlpha;
                break;
            }
        } else if (atst == GaAtst_GreaterOrEqual || atst == GaAtst_Greater) {
            // The canvas discards what fails; what AFAIL keeps of it (FB_ONLY: the colour, the room's and
            // the title's usual GEQUAL 0x7f) is a second pass that keeps only those fragments.
            d.test = gs.test;
            d.failPass = afail;
        }
        // PORT: LESS / LEQUAL / EQUAL / NOTEQUAL (none seen in CT3's ga controls) are drawn untested.
    }
    d.zWritable = (gs.zbuf & kZbufZmsk) == 0;
    if ((gs.test & kTestZte) != 0) {
        switch (static_cast<int>(gs.test >> kTestZtstShift & 3)) {
        case GaZtst_Never:
            d.skip = true;
            return d;
        case GaZtst_Always:
            d.depth = DepthTest::Always;
            break;
        case GaZtst_GreaterOrEqual:
            d.depth = DepthTest::GreaterOrEqual;
            break;
        case GaZtst_Greater:
            d.depth = DepthTest::Greater;
            break;
        }
        d.depthWrite = d.zWritable && !colourOnly;
    }
    switch (static_cast<int>(gs.zbuf >> kZbufPsmShift & kZbufPsmMask)) {
    case GaZpsm_Z32:
        d.zMax = kZMaxZ32;
        break;
    case GaZpsm_Z24:
        d.zMax = kZMaxZ24;
        break;
    default:
        d.zMax = kZMaxZ16;
        break;
    }
    return d;
}

// TEX0's TFX and TCC.
constexpr int kTex0TfxShift = 35;
constexpr int kTex0TfxMask = 3;
constexpr int kTex0TccShift = 34;
enum GaTfx
{
    GaTfx_Modulate = 0,
    GaTfx_Decal = 1,
    GaTfx_Highlight = 2,
    GaTfx_Highlight2 = 3,
};
constexpr std::uint8_t kGsOne = 0x80; // a vertex colour of 1.0 on the GS's scale

std::uint16_t gaCanvasX(float windowX, float anchorShift = 0.0f)
{
    const float x = (windowX / kGaWindowUnit - kGaCentreX + kCanvasCentre + anchorShift) * kGaWindowUnit;
    return static_cast<std::uint16_t>(std::clamp(x, 0.0f, kWindowMax));
}

std::uint16_t gaCanvasY(float windowY)
{
    const float y = ((windowY / kGaWindowUnit - kGaCentreY) * (kCanvasFrameHeight / kGaFrameHeight) + kCanvasCentre) *
                    kGaWindowUnit;
    return static_cast<std::uint16_t>(std::clamp(y, 0.0f, kWindowMax));
}

// The GS's own window: the ROM draws its backdrops as quads the size of the 512 x 448 frame (the title's
// panels, -24..522 at most) and leaves the rest to SCISSOR_1. Squeezed at a wide aspect, that frame is the
// 4:3 box about the centre, and the canvas scissors there: past it the quads would end short, each edge a
// seam, and the larger ones (the sky, the fog bands) would show over nothing. The user's YamiPan decision,
// taken for CT3 (open-items.md "Decisions" 2026-11-11): textured backdrops stay 4:3. The 3D pass, widened
// from DisplayGeometry, is another matter (open-items.md item 8). Rounded inwards: a pixel of picture
// lost rather than a sliver of a quad's edge.
std::uint64_t gaScissor(float squeeze)
{
    const float half = kCanvasFrameWidth / 2.0f;
    const auto x0 = static_cast<std::uint64_t>(std::ceil(half - half * squeeze));
    const auto x1 = static_cast<std::uint64_t>(std::floor(half + half * squeeze)) - 1;
    const auto y1 = static_cast<std::uint64_t>(kCanvasFrameHeight) - 1;
    return x0 | x1 << kScissorX1Shift | y1 << kScissorY1Shift; // y0 = 0
}

// A shadow overlay vertex's x (alyssapan_drawga.h AlyssaPanGaShadowVertex, GS frame pixels) on the canvas: the
// frame's edges at the window's, as the overlay covers what the widened 3D view covers; a point inside the
// frame (a fan's centre: the shadow's anchor, which the game projects as the 3D view does) squeezed with the
// meshes.
std::uint16_t gaShadowCanvasX(float frameX, float squeeze)
{
    const float half = kCanvasFrameWidth / 2.0f;
    if (frameX <= 0.0f || frameX >= kCanvasFrameWidth)
        frameX = half + (frameX - half) / squeeze;
    return gaCanvasX((kGaCentreX - half + frameX) * kGaWindowUnit);
}

std::uint16_t gaShadowCanvasY(float frameY)
{
    return gaCanvasY((kGaCentreY - kGaFrameHeight / 2.0f + frameY) * kGaWindowUnit);
}

// The whole canvas: what the widened 3D pass draws into (DisplayGeometry's picture, at any aspect).
std::uint64_t gaScissorWide()
{
    const auto x1 = static_cast<std::uint64_t>(kCanvasFrameWidth) - 1;
    const auto y1 = static_cast<std::uint64_t>(kCanvasFrameHeight) - 1;
    return x1 << kScissorX1Shift | y1 << kScissorY1Shift; // x0 = y0 = 0
}

// Which ga meshes belong to the widened 3D view: the depth-tested ones (the room, its characters, the
// depth pre-pass). The title's backdrops and the HUD draw with ZTST ALWAYS or no Z test and keep the 4:3
// box (open-items.md "Decisions" 2026-11-11). PORT: a heuristic of the host's, not a ROM flag; a 2D overlay
// drawn depth-tested would show past the box where its quads reach.
bool gaWidened(const GaDraw &draw)
{
    using alyssapan::rendering::DepthTest;
    return draw.depthOnly || draw.depth == DepthTest::GreaterOrEqual || draw.depth == DepthTest::Greater;
}

// The in-game menu is such an overlay (open-items.md nu1): MENU.BIN and the floor map (M101.BIN ..) draw
// depth-tested like the room, and the map's parchment is a quad of frame x -265..473 centred on the map, so
// widened it reached past the box on the right and ended short of the window on the left. Its banks keep
// the 4:3 box with the title (Decisions 2026-11-11): black pillars, nothing stretched.
bool gaWidened(const alyssapan::rendering::GaMeshRecorder::Mesh &mesh, const GaDraw &draw)
{
    return mesh.role != alyssapan::rendering::GaMeshRecorder::BankRole::Menu && gaWidened(draw);
}

// Where a HUD mesh goes at a wide aspect (open-items.md nt1). The HUD is the INFO.BIN bank's (the panic
// ornament top left), drawn through main_rtn like the room: depth-tested at one flat Z near the eye, from
// the ROM's 4:3 view, so the canvas would keep it in the 4:3 box while the widened 3D view fills the
// window. The user's rule anchors 2D to the window from DisplayGeometry (the canvas's squeeze): a HUD mesh
// centred in the frame's left third keeps its distance to the left edge, one in the right third to the
// right edge, the middle third (centred messages) stays centred. PORT: the host's anchors; the ROM has one
// 512-wide frame. Other banks' 2D (the in-game menu, the title) keeps the 4:3 box (Decisions 2026-11-11).
enum class GaAnchor
{
    Centre,
    Left,
    Right,
};

constexpr float kAnchorThird = kCanvasFrameWidth / 6.0f; // a third of the frame, from its centre

// A mesh's drawn x extent in frame pixels about the frame's centre.
struct GaExtent
{
    float x0 = std::numeric_limits<float>::max();
    float x1 = std::numeric_limits<float>::lowest();

    bool empty() const { return x0 > x1; }
};

GaExtent gaExtentOf(const alyssapan::rendering::GaMeshRecorder::Mesh &mesh)
{
    GaExtent e;
    for (std::size_t t = 0; t < mesh.drawn.size(); ++t) {
        if (!mesh.drawn[t])
            continue;
        for (int k = 0; k < 3; ++k) {
            const auto &v = mesh.vertices[mesh.triangles[t * 3 + k]];
            const float x = v.screen[0] / kGaWindowUnit - kGaCentreX;
            e.x0 = std::min(e.x0, x);
            e.x1 = std::max(e.x1, x);
        }
    }
    return e;
}

GaAnchor gaAnchorOf(const alyssapan::rendering::GaMeshRecorder::Mesh &mesh)
{
    const GaExtent e = gaExtentOf(mesh);
    if (e.empty())
        return GaAnchor::Centre;
    const float centre = (e.x0 + e.x1) / 2.0f;
    if (centre <= -kAnchorThird)
        return GaAnchor::Left;
    if (centre >= kAnchorThird)
        return GaAnchor::Right;
    return GaAnchor::Centre;
}

// The anchored mesh's move in frame pixels: half the width the squeeze adds on each side.
float gaAnchorShift(GaAnchor anchor, float squeeze)
{
    const float band = kCanvasFrameWidth / 2.0f * (1.0f / squeeze - 1.0f);
    switch (anchor) {
    case GaAnchor::Left:
        return -band;
    case GaAnchor::Right:
        return band;
    case GaAnchor::Centre:
        break;
    }
    return 0.0f;
}

// CLAMP_1's wrap modes, and REGION_REPEAT's fields: u = (u & MINU) | MAXU, v = (v & MINV) | MAXV.
enum GaWrap
{
    GaWrap_Repeat = 0,
    GaWrap_Clamp = 1,
    GaWrap_RegionClamp = 2,
    GaWrap_RegionRepeat = 3,
};
constexpr int kClampWmtShift = 2;
constexpr std::uint64_t kClampWrapMask = 3;
constexpr int kClampMinUShift = 4;
constexpr int kClampMaxUShift = 14;
constexpr int kClampMinVShift = 24;
constexpr int kClampMaxVShift = 34;
constexpr std::uint64_t kClampFieldMask = 0x3ff;

// How a mesh's texture is sampled on the canvas. The title's sky and fog bands (ATLD) tile one 128 x 128
// cell of a 256 x 256 picture with REGION_REPEAT across st -0.7..1.7; the canvas knows only repeat and
// clamp, and clamped they smeared the picture's edge texels into flat bands (the lighter band at the
// right). A REGION_REPEAT whose mask is a power of two less one, with its fix outside the mask, is the
// plain repeat of that cell: the cell becomes its own image, sampled with REPEAT, its u and v unchanged
// (the canvas divides by the bound image's size, so u / cell width wraps where u & MSK does), moved by
// whole cells past 0 because the canvas's u and v are unsigned.
struct GaSampling
{
    std::shared_ptr<alyssapan::rendering::Image> image;
    std::uint64_t clamp = 0;
    float shiftU = 0.0f; // 12.4 texels
    float shiftV = 0.0f;
};

struct GaWrapCell
{
    bool repeats = false; // REPEAT, or a REGION_REPEAT that is one
    int offset = 0;
    int size = 0;
};

GaWrapCell gaWrapCell(int wrap, std::uint64_t mask, std::uint64_t fix, int imageSize)
{
    if (wrap == GaWrap_Repeat)
        return {true, 0, imageSize};
    const bool cell = wrap == GaWrap_RegionRepeat && std::has_single_bit(mask + 1) && (fix & mask) == 0 &&
                      fix + mask + 1 <= static_cast<std::uint64_t>(imageSize);
    if (!cell)
        return {};
    return {true, static_cast<int>(fix), static_cast<int>(mask + 1)};
}

GaSampling gaSampling(const alyssapan::rendering::GaMeshRecorder::Mesh &mesh)
{
    using alyssapan::rendering::GaMeshRecorder;
    GaSampling sampling{mesh.image, mesh.clamp};
    const std::uint64_t clamp = mesh.clamp;
    const GaWrapCell u = gaWrapCell(static_cast<int>(clamp & kClampWrapMask), clamp >> kClampMinUShift & kClampFieldMask,
                                    clamp >> kClampMaxUShift & kClampFieldMask, mesh.image->width);
    const GaWrapCell v = gaWrapCell(static_cast<int>(clamp >> kClampWmtShift & kClampWrapMask),
                                    clamp >> kClampMinVShift & kClampFieldMask,
                                    clamp >> kClampMaxVShift & kClampFieldMask, mesh.image->height);
    // PORT: a mesh that repeats along one axis and clamps along the other keeps the canvas's single mode
    // (its U's); none on the title.
    if (!u.repeats || !v.repeats)
        return sampling;
    auto image = alyssapan::Application::instance().renderer().textures().regionOf(mesh.image, u.offset, v.offset,
                                                                                    u.size, v.size);
    if (image == nullptr)
        return sampling;
    sampling.image = std::move(image);
    sampling.clamp = GaWrap_Repeat | GaWrap_Repeat << kClampWmtShift;

    float minU = 0.0f, minV = 0.0f;
    for (const GaMeshRecorder::Vertex &vertex : mesh.vertices) {
        minU = std::min(minU, vertex.drawSt[0] * static_cast<float>(mesh.image->width) * kGaWindowUnit);
        minV = std::min(minV, vertex.drawSt[1] * static_cast<float>(mesh.image->height) * kGaWindowUnit);
    }
    const float periodU = static_cast<float>(u.size) * kGaWindowUnit;
    const float periodV = static_cast<float>(v.size) * kGaWindowUnit;
    sampling.shiftU = std::ceil(-minU / periodU) * periodU;
    sampling.shiftV = std::ceil(-minV / periodV) * periodV;
    return sampling;
}

// A shadow pass entry (GaMeshRecorder::Mesh::Shadow; alyssapan_drawga.h AlyssaPan_DrawGaShadow*), over the whole
// canvas as the widened 3D view: the stencil's count reset to its base; a volume mesh counted where it passes the
// frame's depth (the ROM's GEQUAL, no Z write, no colour); the overlay drawn where the count is above the base,
// its vertex colours taken off the frame (Cd - Cs) or added to it.
void DrawGaShadow(alyssapan::rendering::UiCanvas &ui, const void *list,
                  const alyssapan::rendering::GaMeshRecorder::Mesh &mesh, std::uint64_t wide)
{
    using alyssapan::rendering::DepthTest;
    using alyssapan::rendering::StencilMode;
    using Shadow = alyssapan::rendering::GaMeshRecorder::Mesh::Shadow;
    // ALPHA_1 (A, B, C, D at 0, 2, 4, 6): Cd - Cs * As (A 0, B Cs, C As, D Cd) and Cs * As + Cd.
    constexpr std::uint64_t kAlphaSubtract = 2 | 0 << 2 | 0 << 4 | 1 << 6;
    constexpr std::uint64_t kAlphaAdd = 0 | 2 << 2 | 0 << 4 | 1 << 6;

    ui.setRegister(list, kRegScissor1, wide);
    switch (mesh.shadow) {
    case Shadow::None:
        return;
    case Shadow::Start:
        ui.resetStencil(list);
        return;
    case Shadow::Volume: {
        const GaDraw draw = gaDrawOf(mesh.gs, false);
        ui.setRegister(list, kRegTest1, 0);
        ui.setRegister(list, kRegFrame1, 0);
        ui.setDepth(list, draw.depth, false, true, 1.0f / draw.zMax);
        ui.setStencil(list, mesh.shadowSign > 0 ? StencilMode::ShadowIncrement : StencilMode::ShadowDecrement);
        ui.setPrimitive(list, kPrimTriangle, nullptr);
        ui.begin(list, ALYSSAPAN_UIV_RGBAQ);
        const auto vertexAt = [&](std::size_t index) {
            const auto &v = mesh.vertices[index];
            AlyssaPanUiVertex out{};
            out.x = gaCanvasX(v.screen[0]);
            out.y = gaCanvasY(v.screen[1]);
            out.z = static_cast<std::uint32_t>(std::clamp(v.screen[2], 0.0f, draw.zMax));
            return out;
        };
        const auto before = [](const AlyssaPanUiVertex &a, const AlyssaPanUiVertex &b) {
            return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
        };
        const auto emit = [&](AlyssaPanUiVertex a, AlyssaPanUiVertex b, AlyssaPanUiVertex c) {
            std::array<AlyssaPanUiVertex, 3> tri{a, b, c};
            std::sort(tri.begin(), tri.end(), before);
            for (const AlyssaPanUiVertex &v : tri)
                ui.addVertex(list, v);
        };
        // PORT: asShadowPolygonPositions gives each lit face of the caster its own prism, so the wall two
        // neighbouring faces share is stocked twice, once counting up and once down, and must cancel. The
        // two copies run the edge in opposite directions, and the quads (far a, near a, near b, far b) are
        // split along different diagonals ((0 1 2) (2 3 0)): where the wall meets the floor, the host's
        // depth test of the two then differs in the last bit, and the count leaves light seams across the
        // shadow. Split on the diagonal through the quad's least corner, each triangle's corners in one
        // order: both copies then rasterize alike, as the ROM's buffer needs.
        const std::size_t count = mesh.drawn.size();
        for (std::size_t t = 0; t < count; ++t) {
            const std::uint16_t *tri = &mesh.triangles[t * 3];
            const bool quad = t + 1 < count && mesh.drawn[t] && mesh.drawn[t + 1] && tri[3] == tri[2] &&
                              tri[5] == tri[0];
            if (quad) {
                const std::array<AlyssaPanUiVertex, 4> q{vertexAt(tri[0]), vertexAt(tri[1]), vertexAt(tri[2]),
                                                         vertexAt(tri[4])};
                const auto least = std::min_element(q.begin(), q.end(), before) - q.begin();
                const std::size_t d = static_cast<std::size_t>(least) & 1; // 0: diagonal q0-q2, 1: q1-q3
                emit(q[d], q[d + 1], q[d + 2]);
                emit(q[d], q[d + 2], q[(d + 3) & 3]);
                ++t;
                continue;
            }
            if (mesh.drawn[t])
                emit(vertexAt(tri[0]), vertexAt(tri[1]), vertexAt(tri[2]));
        }
        break;
    }
    case Shadow::Overlay: {
        const float squeeze = ui.squeezeFactor();
        ui.setRegister(list, kRegAlpha1, mesh.shadowLighten ? kAlphaAdd : kAlphaSubtract);
        ui.setRegister(list, kRegTest1, 0);
        ui.setRegister(list, kRegFrame1, kFrameFbmskAlpha); // PORT: the frame's alpha is left as it was
        ui.setDepth(list, DepthTest::Off, false, false, 0.0f);
        ui.setStencil(list, StencilMode::ShadowTest);
        ui.setPrimitive(list, kPrimTriangle | kPrimGouraud | kPrimBlended, nullptr);
        ui.begin(list, ALYSSAPAN_UIV_RGBAQ);
        for (const auto &v : mesh.vertices) {
            AlyssaPanUiVertex out{};
            out.x = gaShadowCanvasX(v.screen[0], squeeze);
            out.y = gaShadowCanvasY(v.screen[1]);
            std::memcpy(out.rgba, v.drawRgba.data(), sizeof(out.rgba));
            ui.addVertex(list, out);
        }
        break;
    }
    }
    ui.setStencil(list, StencilMode::Off);
}

// One game frame (gaWaitVSync's swap to the next: GaMeshRecorder::swap) goes to the canvas as one layer, as
// the VU1 program would have sent it (window coordinates and Z, RGBAQ, ST), each mesh with the texture bound
// and the GS state set when it came (gaDrawOf): depth-tested against the frame's Z buffer, cleared where the
// game clears it. Squeezed about the centre at wide aspects: the projection widened, not stretched. The 3D
// view reaches past the 4:3 box (the game culls and clips to the widened view: AlyssaPan_GaWiden), so its
// meshes draw over the whole canvas, and so do a scene frame's fills (a fade must cover what it fades);
// the rest keeps gaScissor's box. No fog yet (open-items.md item 7).
void DrawGaFrame(const std::vector<alyssapan::rendering::GaMeshRecorder::Mesh> &meshes)
{
    using alyssapan::rendering::GaMeshRecorder;
    const void *list = &GaMeshRecorder::instance();
    auto &ui = canvas();

    const std::uint64_t boxed = gaScissor(ui.squeezeFactor());
    const std::uint64_t wide = gaScissorWide();
    bool scene = false;
    for (const GaMeshRecorder::Mesh &mesh : meshes) {
        if (!mesh.fill && !mesh.depthClear && mesh.copy == GaMeshRecorder::Mesh::Copy::None &&
            gaWidened(mesh, gaDrawOf(mesh.gs, mesh.image != nullptr))) {
            scene = true;
            break;
        }
    }

    ui.setSqueezed(list, true);
    ui.setRegister(list, kRegScissor1, boxed);
    ui.setRegister(list, kRegTex11, kGaTex1Linear);
    for (const GaMeshRecorder::Mesh &mesh : meshes) {
        if (mesh.shadow != GaMeshRecorder::Mesh::Shadow::None) {
            DrawGaShadow(ui, list, mesh, wide);
            continue;
        }
        if (mesh.fill) {
            // Over the whole frame: the window in a scene, else gaScissor's 4:3 box as the GS's window holds it.
            ui.setRegister(list, kRegScissor1, scene ? wide : boxed);
            ui.fillColor(list, mesh.fillRgba, mesh.fillFix);
            continue;
        }
        if (mesh.depthClear) {
            ui.clearDepth(list);
            continue;
        }
        if (mesh.copy != GaMeshRecorder::Mesh::Copy::None) {
            // PORT: the canvas has no render-target copy yet (open-items.md): captures and copies are
            // recorded in order and skipped here.
            continue;
        }
        const GaDraw draw = gaDrawOf(mesh.gs, mesh.image != nullptr);
        if (draw.skip)
            continue;
        const bool textured = draw.textured;
        const GaSampling sampling = textured ? gaSampling(mesh) : GaSampling{};
        const int tfx = static_cast<int>(mesh.tex0 >> kTex0TfxShift) & kTex0TfxMask;
        const bool texelAlpha = (mesh.tex0 >> kTex0TccShift & 1) != 0;
        // The HUD's pieces in a side third anchor to that window edge (gaAnchorOf).
        const float anchorShift = mesh.role == GaMeshRecorder::BankRole::Hud
                                      ? gaAnchorShift(gaAnchorOf(mesh), ui.squeezeFactor())
                                      : 0.0f;
        ui.setRegister(list, kRegScissor1, gaWidened(mesh, draw) || anchorShift != 0.0f ? wide : boxed);
        ui.setRegister(list, kRegAlpha1, draw.alpha);
        ui.setRegister(list, kRegTest1, draw.test);
        ui.setRegister(list, kRegFrame1, draw.frame);
        ui.setDepth(list, draw.depth, draw.depthWrite, draw.depthOnly, 1.0f / draw.zMax);
        const std::uint64_t prim = kPrimTriangle | (draw.gouraud ? kPrimGouraud : 0) |
                                   (textured ? kPrimTextured : 0) | (draw.blended ? kPrimBlended : 0);
        if (textured) {
            ui.setRegister(list, kRegTex01, mesh.tex0); // TFX / TCC: HIGHLIGHT adds the lit meshes' specular
            ui.setRegister(list, kRegClamp1, sampling.clamp);
            ui.bindImage(list, sampling.image);
            ui.setPrimitive(list, prim, nullptr);
            ui.begin(list, ALYSSAPAN_UIV_RGBAQ | ALYSSAPAN_UIV_UV);
        } else {
            ui.setPrimitive(list, prim, nullptr);
            ui.begin(list, ALYSSAPAN_UIV_RGBAQ);
        }
        const float texelsU = textured ? static_cast<float>(mesh.image->width) * kGaWindowUnit : 0.0f;
        const float texelsV = textured ? static_cast<float>(mesh.image->height) * kGaWindowUnit : 0.0f;
        const auto emitTriangles = [&] {
            for (std::size_t t = 0; t < mesh.drawn.size(); ++t) {
                if (!mesh.drawn[t])
                    continue;
                for (int k = 0; k < 3; ++k) {
                    const GaMeshRecorder::Vertex &v = mesh.vertices[mesh.triangles[t * 3 + k]];
                    AlyssaPanUiVertex out{};
                    std::memcpy(out.rgba, v.drawRgba.data(), sizeof(out.rgba));
                    if (textured && tfx == GaTfx_Decal) {
                        // DECAL: the texel as it is; its alpha when TCC says so, the vertex's otherwise.
                        out.rgba[0] = out.rgba[1] = out.rgba[2] = kGsOne;
                        if (texelAlpha)
                            out.rgba[3] = kGsOne;
                    }
                    out.x = gaCanvasX(v.screen[0], anchorShift);
                    out.y = gaCanvasY(v.screen[1]);
                    // The GS clamps Z to its buffer's range (a float past 2^32 has no integer to cast to).
                    out.z = static_cast<std::uint32_t>(std::clamp(v.screen[2], 0.0f, draw.zMax));
                    out.u = static_cast<std::uint16_t>(
                        std::clamp(v.drawSt[0] * texelsU + sampling.shiftU, 0.0f, kTexelMax));
                    out.v = static_cast<std::uint16_t>(
                        std::clamp(v.drawSt[1] * texelsV + sampling.shiftV, 0.0f, kTexelMax));
                    ui.addVertex(list, out);
                }
            }
        };
        emitTriangles();

        if (draw.failPass != GaAfail_Keep) {
            // The fragments that failed the alpha test, with what AFAIL still writes for them. PORT: after the
            // mesh's passing ones rather than in their own triangle's order.
            const float scale = 1.0f / draw.zMax;
            ui.setAlphaFailPass(list, true);
            switch (draw.failPass) {
            case GaAfail_FrameOnly:
                ui.setDepth(list, draw.depth, false, false, scale);
                break;
            case GaAfail_RgbOnly:
                ui.setRegister(list, kRegFrame1, draw.frame | kFrameFbmskAlpha);
                ui.setDepth(list, draw.depth, false, false, scale);
                break;
            case GaAfail_DepthOnly:
                ui.setDepth(list, draw.depth, draw.zWritable, true, scale);
                break;
            }
            ui.begin(list, textured ? ALYSSAPAN_UIV_RGBAQ | ALYSSAPAN_UIV_UV : ALYSSAPAN_UIV_RGBAQ);
            emitTriangles();
            ui.setAlphaFailPass(list, false);
        }
    }
    const void *lists[] = {list};
    ui.kick(std::span(lists, 1), alyssapan::ee::Kernel::instance().interrupts().lastFieldTime());
}

} // namespace

float AlyssaPan_GaWiden(void)
{
    return 1.0f / canvas().squeezeFactor();
}

void AlyssaPan_DrawGaObject(const AlyssaPanGaObject *object)
{
    auto &recorder = alyssapan::rendering::GaMeshRecorder::instance();
    if (recorder.record(*object))
        DrawGaFrame(recorder.lastFrame());
}

void AlyssaPan_DrawGaFill(uint32_t rgba, int alphaFix)
{
    auto &recorder = alyssapan::rendering::GaMeshRecorder::instance();
    if (recorder.recordFill(rgba, alphaFix))
        DrawGaFrame(recorder.lastFrame());
}

void AlyssaPan_DrawGaCapture(uint32_t fbp, int width, int height)
{
    using Mesh = alyssapan::rendering::GaMeshRecorder::Mesh;
    auto &recorder = alyssapan::rendering::GaMeshRecorder::instance();
    Mesh copy;
    copy.copy = Mesh::Copy::Capture;
    copy.copyFbp = fbp;
    copy.copyWidth = width;
    copy.copyHeight = height;
    if (recorder.recordCopy(copy))
        DrawGaFrame(recorder.lastFrame());
}

void AlyssaPan_DrawGaCopy(uint32_t fbp, int texWidth, int texHeight, float x0, float y0, float x1, float y1,
                          int alphaFix)
{
    constexpr int kFixMax = 0xff; // ALPHA_1's FIX is a byte
    using Mesh = alyssapan::rendering::GaMeshRecorder::Mesh;
    auto &recorder = alyssapan::rendering::GaMeshRecorder::instance();
    Mesh copy;
    copy.copy = Mesh::Copy::Sprite;
    copy.copyFbp = fbp;
    copy.copyWidth = texWidth;
    copy.copyHeight = texHeight;
    copy.copyRect = {x0, y0, x1, y1};
    copy.copyFix = static_cast<std::uint8_t>(std::clamp(alphaFix, 0, kFixMax));
    if (recorder.recordCopy(copy))
        DrawGaFrame(recorder.lastFrame());
}

void AlyssaPan_DrawGaSwap(void)
{
    auto &recorder = alyssapan::rendering::GaMeshRecorder::instance();
    if (recorder.swap())
        DrawGaFrame(recorder.lastFrame());
}

void AlyssaPan_DrawGaRegister(int reg, uint64_t value)
{
    alyssapan::rendering::GaMeshRecorder::instance().setRegister(reg, value);
}

void AlyssaPan_DrawGaTarget(int toDisplay)
{
    alyssapan::rendering::GaMeshRecorder::instance().setTarget(toDisplay != 0);
}

namespace
{
// The in-game menu's banks, found by a probe of the open menu (open-items.md nu1): MENU.BIN (bank 0 then)
// and the floor map of the area, M<floor><nn>.BIN (bank 4; the pack has M101..M504). MAYN, MCLS and MT*
// are not matched: what they hold is not yet seen.
bool gaIsMenuBank(std::string_view file)
{
    constexpr std::string_view kMenuFile = "MENU.BIN";
    constexpr std::string_view kMapSuffix = ".BIN";
    constexpr std::size_t kMapDigits = 3;
    if (file == kMenuFile)
        return true;
    if (file.size() != 1 + kMapDigits + kMapSuffix.size() || file.front() != 'M' || !file.ends_with(kMapSuffix))
        return false;
    return std::all_of(file.begin() + 1, file.begin() + 1 + kMapDigits,
                       [](char c) { return c >= '0' && c <= '9'; });
}
} // namespace

void AlyssaPan_DrawGaBank(int bank, const char *file)
{
    using Role = alyssapan::rendering::GaMeshRecorder::BankRole;
    // The HUD's bank (the panic ornament top left), found by a probe on the NEW path (open-items.md nt1).
    constexpr std::string_view kHudFile = "INFO.BIN";
    Role role = Role::Scene;
    if (file != nullptr && kHudFile == file)
        role = Role::Hud;
    else if (file != nullptr && gaIsMenuBank(file))
        role = Role::Menu;
    alyssapan::rendering::GaMeshRecorder::instance().setBank(bank, role);
}

void AlyssaPan_DrawGaClearDepth(void)
{
    auto &recorder = alyssapan::rendering::GaMeshRecorder::instance();
    if (recorder.recordDepthClear())
        DrawGaFrame(recorder.lastFrame());
}

void AlyssaPan_DrawGaShadowStart(void)
{
    auto &recorder = alyssapan::rendering::GaMeshRecorder::instance();
    if (recorder.recordShadowStart())
        DrawGaFrame(recorder.lastFrame());
}

void AlyssaPan_DrawGaShadowOverlay(const AlyssaPanGaShadowVertex *vertices, int count, int lighten, int alphaFix,
                                   int foldAdd, int foldSubtract)
{
    auto &recorder = alyssapan::rendering::GaMeshRecorder::instance();
    if (recorder.recordShadowOverlay(vertices, count, lighten != 0, alphaFix, foldAdd, foldSubtract))
        DrawGaFrame(recorder.lastFrame());
}

// TEX0's TW / TH / PSM, the frame the picture is read through.
void AlyssaPan_DrawGaTexture(const AlyssaPanGaTexture *texture)
{
    constexpr int kTwShift = 26, kThShift = 30, kPsmShift = 20, kTccShift = 34;
    constexpr std::uint64_t kLog2Mask = 0xf, kPsmMask = 0x3f;
    auto &recorder = alyssapan::rendering::GaMeshRecorder::instance();
    const std::uint64_t tex0 = texture->tex0;
    alyssapan::rendering::Renderer &renderer = alyssapan::Application::instance().renderer();
    std::optional<alyssapan::Profiler::Scope> phase(std::in_place, renderer.profiler(),
                                                    alyssapan::Profiler::Phase::Textures);
    std::shared_ptr<alyssapan::rendering::Image> image =
        renderer.textures().imageOfGsTexture(
            static_cast<int>(tex0 >> kPsmShift & kPsmMask), texture->width, texture->height,
            static_cast<int>(tex0 >> kTwShift & kLog2Mask), static_cast<int>(tex0 >> kThShift & kLog2Mask),
            texture->pixels, texture->clut, (tex0 >> kTccShift & 1) != 0);
    phase.reset();
    recorder.setTexture(std::move(image), tex0, texture->clamp);
}

void AlyssaPan_DrawUiKick(const void *const *lists, int count)
{
    // The frame is kicked right after the field that ended its wait.
    canvas().kick(std::span(lists, static_cast<std::size_t>(count)),
                  alyssapan::ee::Kernel::instance().interrupts().lastFieldTime());
}
