#include "alyssapan/rendering/ga_mesh.h"

#include <cstring>

#include "alyssapan/core/logger.h"
#include "alyssapan/ee/interrupts.h"
#include "alyssapan/ee/kernel.h"

namespace alyssapan::rendering
{

namespace
{

// The ST and colour blocks start with one qword of VIF codes (STMASK / UNPACK) before their data.
constexpr std::size_t kBlockHead = 16;

} // namespace

GaMeshRecorder &GaMeshRecorder::instance()
{
    static GaMeshRecorder recorder;
    return recorder;
}

void GaMeshRecorder::record(const AlyssaPanGaObject &object)
{
    const std::uint64_t field = ee::Kernel::instance().interrupts().fieldCount();
    if (field != m_field) {
        m_field = field;
        m_last.swap(m_frame);
        m_frame.clear();
    }
    if (object.vertexCount <= 0 || object.positions == nullptr)
        return;

    Mesh mesh;
    mesh.light = object.light;
    mesh.flags = object.flags;
    mesh.gsReg70 = object.gsReg70;
    mesh.gsReg78 = object.gsReg78;
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

    // Each primitive is four halfwords, vertex offsets in qwords (x 0x10); the fourth repeats the first
    // in every record read so far (ATLD's quad: 0 1 2 0 / 3 2 1 3): a triangle.
    if (object.indices != nullptr) {
        for (int p = 0; p < object.primCount; ++p) {
            const std::uint16_t *rec = object.indices + p * 4;
            for (int k = 0; k < 3; ++k) {
                const std::uint16_t index = static_cast<std::uint16_t>(rec[k] >> 4);
                if (index >= mesh.vertices.size())
                    return; // not a record this reading understands: keep nothing of it
                mesh.triangles.push_back(index);
            }
        }
    }

    if (!m_loggedFirst) {
        m_loggedFirst = true;
        const Vertex &v0 = mesh.vertices.front();
        log::info("[ga] first object: light{} flags {:#04x}, {} vertices, {} triangles, v0 ({} {} {}) st ({} {})",
                  mesh.light, mesh.flags, mesh.vertices.size(), mesh.triangles.size() / 3, v0.position[0],
                  v0.position[1], v0.position[2], v0.st[0], v0.st[1]);
    }
    m_frame.push_back(std::move(mesh));
}

} // namespace alyssapan::rendering
