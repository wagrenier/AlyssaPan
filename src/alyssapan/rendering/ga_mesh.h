// The host side of CT3's ga draw objects (alyssapan_drawga.h): the frame's objects, copied out of the
// bank as the game hands them over (positions, ST, colours, triangles, the object's matrix). Started
// 2026-11-10 (ga/gaBGT_DrawObject.c slice 1) as a recorder; slice 2 (2026-11-10) adds the light0 VU1
// program's maths (vu1_object_draw_program 0x04d, the transform, and 0x0f5, the projection; architecture.md
// "The VU1 object program"): each vertex leaves here where the ROM's XYZF2 / RGBAQ / STQ would put it.
// Light1's lighting (0x296, 0x40b) is not read: its meshes are projected the same way, their colour grey.
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "alyssapan/rendering/alyssapan_drawga.h"

namespace alyssapan::rendering
{

struct Image;

class GaMeshRecorder
{
public:
    struct Vertex
    {
        std::array<float, 4> position;
        std::array<float, 2> st;
        std::array<std::uint8_t, 4> rgba;

        // What the VU1 program sends to the GS (light0 maths): window x, y in 12.4 (2048 x 16 the centre),
        // Z, fog; ST after the offset (the GS divides the STQ back to this); RGBA clamped to 0..255.
        std::array<float, 4> screen{};
        std::array<float, 2> drawSt{};
        std::array<std::uint8_t, 4> drawRgba{};
        bool inFront = false; // view z > 0: the projection's divide makes sense
    };

    struct Mesh
    {
        int light = 0;
        int flags = 0;
        std::vector<Vertex> vertices;
        std::vector<std::uint16_t> triangles; // three vertex indices each
        std::vector<bool> drawn;              // per triangle: not dropped by the variant's cull
        std::array<float, 4> position{};
        std::array<float, 16> matrix{};
        std::uint64_t gsReg70 = 0;
        std::uint64_t gsReg78 = 0;

        // The texture bound when the object came (setTexture): null draws it untextured.
        std::shared_ptr<Image> image;
        std::uint64_t tex0 = 0;
        std::uint64_t clamp = 0;

        // A full-screen fill (AlyssaPan_DrawGaFill) instead of a mesh: no vertices, its colour and FIX.
        bool fill = false;
        std::uint32_t fillRgba = 0;
        std::uint8_t fillFix = 0;
    };

    // The texture the objects after it draw with (alyssapan_drawga.h AlyssaPan_DrawGaTexture): the ROM's
    // TEX0_1 / CLAMP_1 and the picture decoded at its source; image null unbinds.
    void setTexture(std::shared_ptr<Image> image, std::uint64_t tex0, std::uint64_t clamp);

    static GaMeshRecorder &instance();

    // The first object after a vblank starts a new frame: what was recorded becomes the last frame's.
    // Answers true when that happened (the caller draws lastFrame()).
    bool record(const AlyssaPanGaObject &object);

    // A full-screen fill in draw order (alyssapan_drawga.h AlyssaPan_DrawGaFill); it closes a frame as an
    // object does, so a screen with nothing but a fade still reaches the display.
    bool recordFill(std::uint32_t rgba, int alphaFix);

    const std::vector<Mesh> &lastFrame() const { return m_last; }

private:
    static void project(Mesh &mesh, const AlyssaPanGaObject &object);
    static void cull(Mesh &mesh);
    // The first draw after a vblank: what was recorded becomes the last frame's. Answers whether it did.
    bool nextField();

    std::vector<Mesh> m_frame;
    std::vector<Mesh> m_last;
    std::uint64_t m_field = 0;
    bool m_loggedFirst = false;
    bool m_loggedTexture = false;
    std::shared_ptr<Image> m_image;
    std::uint64_t m_tex0 = 0;
    std::uint64_t m_clamp = 0;
};

} // namespace alyssapan::rendering
