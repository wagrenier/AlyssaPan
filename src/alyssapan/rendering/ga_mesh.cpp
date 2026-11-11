#include "alyssapan/rendering/ga_mesh.h"

#include <algorithm>
#include <cstring>

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

} // namespace

GaMeshRecorder &GaMeshRecorder::instance()
{
    static GaMeshRecorder recorder;
    return recorder;
}

// vu1_object_draw_program 0x04d (light0's transform: M x v into the vertex's own slot, the colour bytes to
// floats) and 0x0f5 (the projection, after MSCNT): per vertex v (view space),
//   Q = vf18.w / v.z                                  the projection distance over depth
//   XYZ = vf17.xyz + vf16.xyz * (v.x, v.y, 1) * Q     (vf31 adds 0: MSCAL 0, its loader, is never sent)
//   F = (vf17.w + vf29.x) * 16 + vf29.y / (vf29.w + |v|^2)
//   RGBA = clamp(colour + vf20, 0, vf29.z)             vf20 = the object's +0x38 qword
//   STQ = (st + vf30.xy) * Q                           the GS divides it back: st + vf30.xy
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

    for (Vertex &v : mesh.vertices) {
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
        for (int c = 0; c < 4; ++c) {
            // PORT: light1's colours come from its lighting (0x296 / 0x40b, not read): grey until then.
            const float in = mesh.light == ALYSSAPAN_GA_LIGHT0 ? static_cast<float>(v.rgba[c]) + mesh.position[c]
                                                               : 128.0f;
            v.drawRgba[c] = static_cast<std::uint8_t>(std::clamp(in, 0.0f, std::min(fog[2], 255.0f)));
        }
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

bool GaMeshRecorder::record(const AlyssaPanGaObject &object)
{
    bool frameEnded = false;
    const std::uint64_t field = ee::Kernel::instance().interrupts().fieldCount();
    if (field != m_field) {
        m_field = field;
        m_last.swap(m_frame);
        m_frame.clear();
        frameEnded = true;
    }
    if (object.vertexCount <= 0 || object.positions == nullptr)
        return frameEnded;

    Mesh mesh;
    mesh.light = object.light;
    mesh.flags = object.flags;
    mesh.gsReg70 = object.gsReg70;
    mesh.gsReg78 = object.gsReg78;
    mesh.image = m_image;
    mesh.tex0 = m_tex0;
    mesh.clamp = m_clamp;
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
