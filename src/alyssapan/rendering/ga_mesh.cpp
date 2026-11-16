#include "alyssapan/rendering/ga_mesh.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <utility>

#include "alyssapan/core/logger.h"
#include "alyssapan/ee/interrupts.h"
#include "alyssapan/ee/kernel.h"
#include "alyssapan/rendering/frame.h"

namespace alyssapan::rendering
{

namespace
{

// The ST and colour blocks start with one qword of VIF codes (STMASK / UNPACK) before their data.
constexpr std::size_t kBlockHead = 16;

// The projection's I register (0x0f5: loi 16): the fog base goes to XYZF2's F field, 4 bits up.
constexpr float kFogUnit = 16.0f;

// flags & 3 (the program's variant, MSCAL 0x1a + 2 x it).
constexpr int kVariantMask = 3;

// The GS registers the ga packets set that decide a host draw (sce_gs.h's SCE_GS_*).
constexpr int kRegPrmode = 0x1b;
constexpr int kRegAlpha1 = 0x42;
constexpr int kRegTest1 = 0x47;
constexpr int kRegFrame1 = 0x4c;
constexpr int kRegZbuf1 = 0x4e;

// A game frame takes 2 fields (30 Hz), a slow one a few more: 10 without a swap is a loop that has none.
constexpr std::uint64_t kSwaplessFields = 10;

constexpr int kLights = 4; // per kind: four directional, four point (a lane each)
constexpr int kRgba = 4;
constexpr std::uint8_t kGsOne = 0x80; // a colour or alpha of 1.0 on the GS's scale

// The VU's zero flag: it flushes denormals, so a result under the smallest normal float counts as zero.
bool vuZero(float x)
{
    return std::fabs(x) < std::numeric_limits<float>::min();
}

// RSQRT / ERSQRT: the VU has no infinity, a zero answers the largest float (so a point light on the vertex
// adds 0 x max = 0, not a NaN). Its operand is never negative here (a sum of squares).
float vuRsqrt(float x)
{
    return vuZero(x) ? std::numeric_limits<float>::max() : 1.0f / std::sqrt(x);
}

float dot3(const float a[3], const float b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// The colour block's UNPACK V4-8 (the last word of its VIF head qword, 0x6e......): without USN (bit 14 of
// its immediate) the VIF sign-extends each byte into VU memory, so 0x80..0xff arrive as -128..-1 and the
// programs' ITOF0 keeps them negative (the clamp then makes them 0). Every block read so far has USN 0.
bool colourBytesSigned(const void *colourBlock)
{
    constexpr int kCodeWord = 3;
    constexpr std::uint32_t kCmdShift = 24;
    constexpr std::uint32_t kUnpack = 0x60;       // CMD 011m vnvl
    constexpr std::uint32_t kUnpackV4_8 = 0x0e;   // vn 3, vl 2
    constexpr std::uint32_t kUnpackUsn = 0x4000;
    if (colourBlock == nullptr)
        return false;
    std::uint32_t code = 0;
    std::memcpy(&code, static_cast<const std::uint8_t *>(colourBlock) + kCodeWord * sizeof(code), sizeof(code));
    const std::uint32_t cmd = code >> kCmdShift;
    if ((cmd & kUnpack) != kUnpack || (cmd & 0x0f) != kUnpackV4_8)
        return false; // not the unpack this reading knows: the bytes as they are
    return (code & kUnpackUsn) == 0;
}

float colourByte(std::uint8_t byte, bool sign)
{
    return sign ? static_cast<float>(static_cast<std::int8_t>(byte)) : static_cast<float>(byte);
}

// The lit programs' inputs for one object (the header's light matrix vf21..vf23, the light work at VU 0x180,
// the header's vf30.w).
struct LitObject
{
    int light = ALYSSAPAN_GA_LIGHT0;
    const float *lightMatrix = nullptr;          // 3 rows of 4
    const AlyssaPanGaLightSet *set = nullptr;
    const std::uint8_t *normals = nullptr;       // vertexCount qwords, x y z w
    bool points = false;                         // a point scale other than 1.0: the full path
    bool highlight = false;                      // vf30.w != 0: the alpha becomes a specular
    float scale = 0.0f;                          // vf30.w, the object's +0x30
};

LitObject litObjectOf(const AlyssaPanGaObject &object)
{
    LitObject lit;
    if (object.light == ALYSSAPAN_GA_LIGHT0 || object.normalBlock == nullptr || object.lightMatrix == nullptr ||
        object.lightWork == nullptr)
        return lit; // light0, or a lit object whose blocks did not resolve: its colours as they are
    lit.light = object.light;
    lit.lightMatrix = object.lightMatrix;
    lit.set = reinterpret_cast<const AlyssaPanGaLightSet *>(static_cast<const std::uint8_t *>(object.lightWork) +
                                                             kBlockHead);
    lit.normals = static_cast<const std::uint8_t *>(object.normalBlock) + kBlockHead;
    // 0x40b / 0x2b9 / 0x3b8: VU 0x187 - 1.0, the four zero flags (FMAND 0xf) all set -> the directional-only
    // path (0x4bc / 0x36b).
    for (int k = 0; k < kLights; ++k)
        lit.points |= !vuZero(lit.set->pointScale[k] - 1.0f);
    // The directional path's end and light1's full path test vf0.x + vf30.w (FMAND 8, Zx); light2's full path
    // (0x3b8) returns without the test.
    lit.highlight = !vuZero(object.lightScale) && !(lit.light == ALYSSAPAN_GA_LIGHT2 && lit.points);
    lit.scale = object.lightScale;
    return lit;
}

// The lit programs' colour for one vertex: what they leave in its colour slot (VU 0x100 + i) for the
// projection, which then adds the ambient (vf20) and clamps (0x0f5 / 0x0af). v is the vertex in the view
// (0x084's transform), colour the block's bytes.
//   normal  0x296 / 0x25a: n = row0 n.x + row1 n.y + row2 n.z, by the light matrix (vf21..vf23)
//   0x4bc / 0x36b (all point scales 1.0):   c = colour + sum_j dirColour[j] d_j
//   0x40b / 0x2b9 (light1, point lights):   c += sum_j pointColour[j] e_j
//   0x3b8 (light2, point lights):           the same with light2's e_j
// with d_j = max(I_j n.x dx_j + I_j n.y dy_j + I_j n.z dz_j, 0) (I = dirView's -w row),
//      p_j = (point_j - v) x scale_j,
//      light1: e_j = max(max(n . p_j, 0) / |p_j| - max(n . p_j, 0), 0)  (a cosine fading linearly to the range)
//      light2: e_j = max(|p_j| - |p_j|^2, 0)                            (0x3dd adds |p|^2, not n . p: no angle)
// The highlight (0x5a8 / 0x4f4) then replaces the alpha: r = v^ - 2 (v^ . n) n, s = max(r . -dir_0, 0),
// alpha = scale x -I_0 x s^4, and clears vf20.w so the ambient adds nothing to it.
std::array<float, kRgba> litColour(const LitObject &lit, const float view[3], const float normal[3],
                                   const std::uint8_t rgba[kRgba], bool sign)
{
    const AlyssaPanGaLightSet &ls = *lit.set;
    const float *L = lit.lightMatrix;
    float n[3];
    for (int k = 0; k < 3; ++k)
        n[k] = L[0 * 4 + k] * normal[0] + L[1 * 4 + k] * normal[1] + L[2 * 4 + k] * normal[2];

    float d[kLights];
    float e[kLights] = {};
    for (int j = 0; j < kLights; ++j) {
        const float i = ls.intensity[j];
        d[j] = std::max(i * n[0] * ls.dir[0][j] + i * n[1] * ls.dir[1][j] + i * n[2] * ls.dir[2][j], 0.0f);
        if (!lit.points)
            continue;
        const float p[3] = {(ls.point[0][j] - view[0]) * ls.pointScale[j],
                            (ls.point[1][j] - view[1]) * ls.pointScale[j],
                            (ls.point[2][j] - view[2]) * ls.pointScale[j]};
        const float sum = dot3(p, p);
        const float dsq = vuZero(sum) ? 0.0f : sum; // a denormal operand reads as 0
        const float inv = vuRsqrt(dsq);
        // The term before its last max: x / |p| - x, x the light's own factor.
        const float x = lit.light == ALYSSAPAN_GA_LIGHT2 ? std::max(dsq, 0.0f) : std::max(dot3(n, p), 0.0f);
        e[j] = std::max(x * inv - x, 0.0f);
    }

    std::array<float, kRgba> c{};
    for (int k = 0; k < kRgba; ++k) {
        float sum = colourByte(rgba[k], sign);
        for (int j = 0; j < kLights; ++j)
            sum += ls.dirColour[j][k] * d[j];
        if (lit.points) {
            for (int j = 0; j < kLights; ++j)
                sum += ls.pointColour[j][k] * e[j];
        }
        c[k] = sum;
    }

    if (lit.highlight) {
        const float q = vuRsqrt(dot3(view, view));
        const float vh[3] = {view[0] * q, view[1] * q, view[2] * q};
        const float vn = -2.0f * vh[0] * n[0] + -2.0f * vh[1] * n[1] + -2.0f * vh[2] * n[2];
        const float r[3] = {vh[0] + n[0] * vn, vh[1] + n[1] * vn, vh[2] + n[2] * vn};
        float s = std::max(r[0] * -ls.dir[0][0] + r[1] * -ls.dir[1][0] + r[2] * -ls.dir[2][0], 0.0f);
        s = s * s;
        s = s * s;
        // 0x5b2: vf30.w x= -I_0 once, then x s^4 per vertex (sq.w: only the alpha is replaced).
        c[3] = lit.scale * -ls.intensity[0] * s;
    }
    return c;
}

} // namespace

GaMeshRecorder &GaMeshRecorder::instance()
{
    static GaMeshRecorder recorder;
    return recorder;
}

// vu1_object_draw_program 0x04d (light0's transform: M x v into the vertex's own slot, the colour bytes to
// floats) or 0x084 and the lighting (lit objects: litColour), then 0x0f5 (the projection, after MSCNT): per
// vertex v (view space),
//   Q = vf18.w / v.z                                  the projection distance over depth
//   XYZ = vf17.xyz + vf16.xyz * (v.x, v.y, 1) * Q     (vf31 adds 0: MSCAL 0, its loader, is never sent)
//   F = (vf17.w + vf29.x) * 16 + vf29.y / (vf29.w + |v|^2)
//   RGBA = clamp(colour + vf20, 0, vf29.z)             vf20 = the object's +0x38 qword (w 0 under a highlight)
//   STQ = (st + vf30.xy) * Q                           the GS divides it back: st + vf30.xy
// The clipped path's programs (0x0af after 0x25a / 0x2b9 / 0x3b8) do the same over the EE's local view.
void GaMeshRecorder::project(Mesh &mesh, const AlyssaPanGaObject &object)
{
    if (object.vuHeader == nullptr)
        return;
    const float *h = object.vuHeader;
    const float *scale = h + ALYSSAPAN_GA_HDR_SCALE;
    const float *offset = h + ALYSSAPAN_GA_HDR_OFFSET;
    const float distance = h[ALYSSAPAN_GA_HDR_COUNTS + 3];
    const float *fog = h + ALYSSAPAN_GA_HDR_FOG;
    const float *stOffset = h + ALYSSAPAN_GA_HDR_ST;
    const float fogBase = (offset[3] + fog[0]) * kFogUnit;
    const float *m = mesh.matrix.data();
    const LitObject lit = litObjectOf(object);
    const bool sign = colourBytesSigned(object.colourBlock);
    const float colourMax = std::min(fog[2], 255.0f);
    // The ambient the projection adds; the highlight routine clears its w (0x5b1: vf20.w = vf0.w x vf0.x).
    std::array<float, kRgba> ambient = mesh.position;
    if (lit.highlight)
        ambient[3] = 0.0f;

    for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
        Vertex &v = mesh.vertices[i];
        const float x = v.position[0], y = v.position[1], z = v.position[2];
        const float vx = m[0] * x + m[4] * y + m[8] * z + m[12];
        const float vy = m[1] * x + m[5] * y + m[9] * z + m[13];
        const float vz = m[2] * x + m[6] * y + m[10] * z + m[14];
        v.inFront = vz > 0.0f;
        const float q = vz != 0.0f ? distance / vz : 0.0f;
        v.screen[0] = offset[0] + scale[0] * vx * q;
        v.screen[1] = offset[1] + scale[1] * vy * q;
        v.screen[2] = offset[2] + scale[2] * q;
        const float d2 = fog[3] + vx * vx + vy * vy + vz * vz;
        v.screen[3] = fogBase + (d2 != 0.0f ? fog[1] / d2 : 0.0f);
        v.drawSt[0] = v.st[0] + stOffset[0];
        v.drawSt[1] = v.st[1] + stOffset[1];

        std::array<float, kRgba> colour{};
        if (lit.set != nullptr) {
            float normal[3];
            std::memcpy(normal, lit.normals + i * sizeof(float) * kRgba, sizeof(normal));
            const float view[3] = {vx, vy, vz};
            colour = litColour(lit, view, normal, v.rgba.data(), sign);
        } else {
            for (int c = 0; c < kRgba; ++c)
                colour[c] = colourByte(v.rgba[c], sign);
        }
        // MAXz vf0 (0), MINIz vf29.z, FTOI0: the GS takes the integer part.
        for (int c = 0; c < kRgba; ++c)
            v.drawRgba[c] = static_cast<std::uint8_t>(std::clamp(colour[c] + ambient[c], 0.0f, colourMax));
    }
}

// The kick routines 0x1fe (variant 0) and 0x22c (variant 1): the screen-space edges' outer product (OPMULA /
// OPMSUB), its z's sign flag (MAC 0x20) sets the third vertex's ADC bit. Variants 2 and 3 (0x1d9) keep all.
void GaMeshRecorder::cull(Mesh &mesh)
{
    const int variant = mesh.flags & kVariantMask;
    const std::size_t count = mesh.triangles.size() / 3;
    mesh.drawn.assign(count, true);
    for (std::size_t t = 0; t < count; ++t) {
        const Vertex &a = mesh.vertices[mesh.triangles[t * 3]];
        const Vertex &b = mesh.vertices[mesh.triangles[t * 3 + 1]];
        const Vertex &c = mesh.vertices[mesh.triangles[t * 3 + 2]];
        if (!a.inFront || !b.inFront || !c.inFront) {
            // PORT: the no-clip path trusts the object to be in view; a vertex behind the eye would wrap on
            // the GS. Dropped here rather than drawn through the projection's sign flip.
            mesh.drawn[t] = false;
            continue;
        }
        const float e1x = b.screen[0] - a.screen[0], e1y = b.screen[1] - a.screen[1];
        const float e2x = c.screen[0] - a.screen[0], e2y = c.screen[1] - a.screen[1];
        const float crossZ = e1x * e2y - e2x * e1y;
        if (variant == ALYSSAPAN_GA_CULL_NEGATIVE)
            mesh.drawn[t] = !(crossZ < 0.0f);
        else if (variant == ALYSSAPAN_GA_CULL_POSITIVE)
            mesh.drawn[t] = crossZ < 0.0f;
    }
}

void GaMeshRecorder::setTexture(std::shared_ptr<Image> image, std::uint64_t tex0, std::uint64_t clamp)
{
    if (!m_loggedTexture && image != nullptr) {
        m_loggedTexture = true;
        log::info("[ga] first texture: {}x{} TEX0 {:#018x} (PSM {:#x}, TFX {}, TCC {}) CLAMP {:#x}", image->width,
                  image->height, tex0, tex0 >> 20 & 0x3f, tex0 >> 35 & 3, tex0 >> 34 & 1, clamp);
    }
    m_image = std::move(image);
    m_tex0 = tex0;
    m_clamp = clamp;
}

void GaMeshRecorder::setRegister(int reg, std::uint64_t value)
{
    switch (reg) {
    case kRegFrame1:
        m_gs.frame = value;
        break;
    case kRegZbuf1:
        m_gs.zbuf = value;
        break;
    case kRegPrmode:
        m_gs.prmode = value;
        m_gs.controlled = true;
        break;
    case kRegTest1:
        m_gs.test = value;
        m_gs.controlled = true;
        break;
    case kRegAlpha1:
        m_gs.alpha = value;
        break;
    default:
        break; // nothing the host draws decides on
    }
}

void GaMeshRecorder::setTarget(bool toDisplay)
{
    m_gs.toDisplay = toDisplay;
}

bool GaMeshRecorder::close(std::uint64_t field)
{
    m_closeField = field;
    m_shadowPass = false; // a shadow pass ends within its frame (asShadowEnd); none carries over
    if (m_frame.empty())
        return false;
    m_last.swap(m_frame);
    m_frame.clear();
    return true;
}

bool GaMeshRecorder::swap()
{
    return close(ee::Kernel::instance().interrupts().fieldCount());
}

bool GaMeshRecorder::closeWithoutSwap()
{
    const std::uint64_t field = ee::Kernel::instance().interrupts().fieldCount();
    if (field - m_closeField < kSwaplessFields)
        return false;
    if (!m_loggedSwapless && !m_frame.empty()) {
        m_loggedSwapless = true;
        log::warning("[ga] {} draws and no gaWaitVSync swap for {} fields: the frame is closed on the field",
                     m_frame.size(), field - m_closeField);
    }
    return close(field);
}

bool GaMeshRecorder::recordDepthClear()
{
    const bool frameEnded = closeWithoutSwap();
    if (!m_gs.toDisplay)
        return frameEnded;
    Mesh mesh;
    mesh.depthClear = true;
    mesh.gs = m_gs;
    m_frame.push_back(std::move(mesh));
    return frameEnded;
}

bool GaMeshRecorder::recordFill(std::uint32_t rgba, int alphaFix)
{
    constexpr int kFixMax = 0xff; // ALPHA_1's FIX is a byte
    const bool frameEnded = closeWithoutSwap();
    if (!m_gs.toDisplay)
        return frameEnded;
    Mesh mesh;
    mesh.fill = true;
    mesh.fillRgba = rgba;
    mesh.fillFix = static_cast<std::uint8_t>(std::clamp(alphaFix, 0, kFixMax));
    m_frame.push_back(std::move(mesh));
    return frameEnded;
}

bool GaMeshRecorder::recordCopy(const Mesh &copy)
{
    // A copy names its own frames: kept whatever the draw buffer is.
    const bool frameEnded = closeWithoutSwap();
    if (!m_loggedCopy) {
        m_loggedCopy = true;
        log::info("[ga] first frame-buffer {}: GS {:#x}, {}x{} (recorded, not drawn yet: no render-target copy)",
                  copy.copy == Mesh::Copy::Capture ? "capture" : "copy", copy.copyFbp, copy.copyWidth,
                  copy.copyHeight);
    }
    m_frame.push_back(copy);
    return frameEnded;
}

bool GaMeshRecorder::recordShadowStart()
{
    const bool frameEnded = closeWithoutSwap();
    m_shadowPass = true;
    m_shadowStep = 0;
    Mesh mesh;
    mesh.shadow = Mesh::Shadow::Start;
    m_frame.push_back(std::move(mesh));
    return frameEnded;
}

// The ROM's shadow buffer, per channel (architecture.md "The shadow overlay"): cleared to 0x80, a face adds or
// subtracts its Cs * As (m_shadowStep), each step clamped to a byte; the fold adds foldAdd and subtracts
// foldSubtract (FIX 0x80), clamped again. The texel T is that of one face's net count; the overlay's
// MODULATE gives T * Cv / 0x80 and its blend that times FIX / 0x80, each clamped and truncated as the GS does.
// PORT: the stencil holds the count, and every count above none takes one face's T. CT3's volume faces
// (ShadowVolume_vertex_color_data 0x20 + ambient_ShadowVolume_color 0x80 at alpha 0x20: 0x28 a face) fold
// to the buffer's top, 0x1f, from one face on, so no count differs.
bool GaMeshRecorder::recordShadowOverlay(const AlyssaPanGaShadowVertex *vertices, int count, bool lighten,
                                         int alphaFix, int foldAdd, int foldSubtract)
{
    constexpr int kByteMax = 0xff;
    constexpr int kGsOneShift = 7;          // 0x80 is 1.0
    constexpr int kShadowBufferClear = 0x80; // asShadowStart's clear sprite (dc_EBclear_C, RGBA 0x80808080)
    const bool frameEnded = closeWithoutSwap();
    const bool wasPass = std::exchange(m_shadowPass, false);
    const int counted = std::min(kShadowBufferClear + m_shadowStep, kByteMax);
    const int folded = std::min(counted + foldAdd, kByteMax);
    const int texel = std::max(folded - foldSubtract, 0);
    if (!wasPass || texel == 0 || count <= 0 || vertices == nullptr)
        return frameEnded;

    Mesh mesh;
    mesh.shadow = Mesh::Shadow::Overlay;
    mesh.shadowLighten = lighten;
    mesh.vertices.resize(static_cast<std::size_t>(count) * 3);
    for (std::size_t i = 0; i < mesh.vertices.size(); ++i) {
        const AlyssaPanGaShadowVertex &in = vertices[i];
        Vertex &v = mesh.vertices[i];
        v.screen[0] = in.x;
        v.screen[1] = in.y;
        for (int c = 0; c < 3; ++c) {
            const int modulated = std::min(texel * in.rgba[c] >> kGsOneShift, kByteMax);
            v.drawRgba[c] = static_cast<std::uint8_t>(std::min(modulated * alphaFix >> kGsOneShift, kByteMax));
        }
        v.drawRgba[3] = kGsOne; // As 1.0: the canvas's blend takes the colour as it is
        mesh.triangles.push_back(static_cast<std::uint16_t>(i));
    }
    mesh.drawn.assign(static_cast<std::size_t>(count), true);
    if (!m_loggedShadow) {
        m_loggedShadow = true;
        const Vertex &v0 = mesh.vertices.front();
        log::info("[ga] first shadow overlay: {} triangles, {} by {} {} {} (a face {:#x}, texel {:#x}, FIX {:#x})",
                  count, lighten ? "lightened" : "darkened", v0.drawRgba[0], v0.drawRgba[1], v0.drawRgba[2],
                  m_shadowStep, texel, alphaFix);
    }
    m_frame.push_back(std::move(mesh));
    return frameEnded;
}

bool GaMeshRecorder::record(const AlyssaPanGaObject &object)
{
    const bool frameEnded = closeWithoutSwap();
    const bool volume = !m_gs.toDisplay && m_shadowPass;
    if (object.vertexCount <= 0 || object.positions == nullptr || (!m_gs.toDisplay && !volume))
        return frameEnded;

    Mesh mesh;
    mesh.light = object.light;
    mesh.bank = m_bank;
    mesh.role = m_role;
    mesh.flags = object.flags;
    mesh.gsReg70 = object.gsReg70;
    mesh.gsReg78 = object.gsReg78;
    mesh.image = m_image;
    mesh.tex0 = m_tex0;
    mesh.clamp = m_clamp;
    mesh.gs = m_gs;
    if (object.position != nullptr)
        std::memcpy(mesh.position.data(), object.position, sizeof(mesh.position));
    if (object.matrix != nullptr)
        std::memcpy(mesh.matrix.data(), object.matrix, sizeof(mesh.matrix));

    const auto *st = object.stBlock != nullptr
                         ? reinterpret_cast<const float *>(static_cast<const std::uint8_t *>(object.stBlock) + kBlockHead)
                         : nullptr;
    const auto *rgba = object.colourBlock != nullptr
                           ? static_cast<const std::uint8_t *>(object.colourBlock) + kBlockHead
                           : nullptr;
    mesh.vertices.resize(static_cast<std::size_t>(object.vertexCount));
    for (int i = 0; i < object.vertexCount; ++i) {
        Vertex &v = mesh.vertices[static_cast<std::size_t>(i)];
        std::memcpy(v.position.data(), object.positions + i * 4, sizeof(v.position));
        if (st != nullptr)
            std::memcpy(v.st.data(), st + i * 2, sizeof(v.st));
        if (rgba != nullptr)
            std::memcpy(v.rgba.data(), rgba + i * 4, sizeof(v.rgba));
    }

    // Each primitive is four halfwords, vertex offsets in qwords (x 0x10): the kick routines read the first
    // three (0x1ba: ilw.x / .y / .z); the fourth repeats the first in every record read so far.
    if (object.indices != nullptr) {
        for (int p = 0; p < object.primCount; ++p) {
            const std::uint16_t *rec = object.indices + p * 4;
            for (int k = 0; k < 3; ++k) {
                const std::uint16_t index = static_cast<std::uint16_t>(rec[k] >> 4);
                if (index >= mesh.vertices.size())
                    return frameEnded; // not a record this reading understands: keep nothing of it
                mesh.triangles.push_back(index);
            }
        }
    }

    project(mesh, object);
    cull(mesh);

    if (volume) {
        // A volume face counts the way its draw control blends (asDrawShadowVolume's SDW_drawctrl_A / _S);
        // what it adds to the ROM's buffer is its colour times its alpha (both faces share the template's).
        constexpr int kGsOneShift = 7;
        mesh.shadow = Mesh::Shadow::Volume;
        mesh.shadowSign = blendModeOf(m_gs.alpha) == BlendMode::Subtractive ? -1 : 1;
        const Vertex &v0 = mesh.vertices.front();
        m_shadowStep = std::max(m_shadowStep, v0.drawRgba[0] * v0.drawRgba[3] >> kGsOneShift);
    }

    if (!m_loggedFirst) {
        m_loggedFirst = true;
        const Vertex &v0 = mesh.vertices.front();
        std::size_t kept = 0;
        for (bool d : mesh.drawn)
            kept += d ? 1 : 0;
        log::info("[ga] first object: light{} flags {:#04x}, {} vertices, {} triangles ({} kept), v0 ({} {} {}) "
                  "st ({} {}) -> window ({} {}) z {} fog {} rgba {} {} {} {}; matrix diag {} {} {} t ({} {} {})",
                  mesh.light, mesh.flags, mesh.vertices.size(), mesh.triangles.size() / 3, kept, v0.position[0],
                  v0.position[1], v0.position[2], v0.st[0], v0.st[1], v0.screen[0] / 16.0f, v0.screen[1] / 16.0f,
                  v0.screen[2], v0.screen[3], v0.drawRgba[0], v0.drawRgba[1], v0.drawRgba[2], v0.drawRgba[3],
                  mesh.matrix[0], mesh.matrix[5], mesh.matrix[10], mesh.matrix[12], mesh.matrix[13],
                  mesh.matrix[14]);
    }
    m_frame.push_back(std::move(mesh));
    return frameEnded;
}

} // namespace alyssapan::rendering
