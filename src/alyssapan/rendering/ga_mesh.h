// The host side of CT3's ga draw objects (alyssapan_drawga.h): the frame's objects, copied out of the
// bank as the game hands them over (positions, ST, colours, triangles, the object's matrix). Started
// 2026-11-10 (ga/gaBGT_DrawObject.c slice 1): it records; the view transform (the VU1 programs' maths)
// and the GPU pass that draws the meshes come with their reading (open-items.md).
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "alyssapan/rendering/alyssapan_drawga.h"

namespace alyssapan::rendering
{

class GaMeshRecorder
{
public:
    struct Vertex
    {
        std::array<float, 4> position;
        std::array<float, 2> st;
        std::array<std::uint8_t, 4> rgba;
    };

    struct Mesh
    {
        int light = 0;
        int flags = 0;
        std::vector<Vertex> vertices;
        std::vector<std::uint16_t> triangles; // three vertex indices each
        std::array<float, 4> position{};
        std::array<float, 16> matrix{};
        std::uint64_t gsReg70 = 0;
        std::uint64_t gsReg78 = 0;
    };

    static GaMeshRecorder &instance();

    // The first object after a vblank starts a new frame: what was recorded becomes the last frame's.
    void record(const AlyssaPanGaObject &object);

    const std::vector<Mesh> &lastFrame() const { return m_last; }

private:
    std::vector<Mesh> m_frame;
    std::vector<Mesh> m_last;
    std::uint64_t m_field = 0;
    bool m_loggedFirst = false;
};

} // namespace alyssapan::rendering
