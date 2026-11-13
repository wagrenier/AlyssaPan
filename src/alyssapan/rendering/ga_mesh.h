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

    // The ga path's GS state as its packets set it (alyssapan_drawga.h AlyssaPan_DrawGaRegister /
    // AlyssaPan_DrawGaTarget): what each mesh draws under. Registers keep their values until set again, as
    // on the GS.
    struct GsState
    {
        bool controlled = false;  // a draw control (PRMODE / TEST_1) has come: before one, a mesh draws as it
                                  // did before the state was read (blended, untested)
        bool toDisplay = true;    // the draw buffer is one of the two display frames
        std::uint64_t frame = 0;  // FRAME_1: FBMSK
        std::uint64_t zbuf = 0;   // ZBUF_1: PSM, ZMSK
        std::uint64_t prmode = 0; // PRMODE (gsinit's PRMODECONT 0: the primitives' attributes come from here)
        std::uint64_t test = 0;   // TEST_1
        std::uint64_t alpha = 0;  // ALPHA_1
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
        GsState gs; // the registers when it came

        // The Z buffer cleared, the colour untouched (AlyssaPan_DrawGaClearDepth) instead of a mesh.
        bool depthClear = false;

        // A full-screen fill (AlyssaPan_DrawGaFill) instead of a mesh: no vertices, its colour and FIX.
        bool fill = false;
        std::uint32_t fillRgba = 0;
        std::uint8_t fillFix = 0;

        // A frame-buffer copy (AlyssaPan_DrawGaCapture / AlyssaPan_DrawGaCopy) instead of a mesh: the GS
        // address it names, the picture's size, and for a copy the window rectangle and FIX. Recorded in
        // draw order; the canvas has no render-target copy yet, so the frame pass skips them.
        enum class Copy : std::uint8_t { None, Capture, Sprite };
        Copy copy = Copy::None;
        std::uint32_t copyFbp = 0;
        int copyWidth = 0;
        int copyHeight = 0;
        std::array<float, 4> copyRect{}; // x0, y0, x1, y1
        std::uint8_t copyFix = 0;
    };

    // The texture the objects after it draw with (alyssapan_drawga.h AlyssaPan_DrawGaTexture): the ROM's
    // TEX0_1 / CLAMP_1 and the picture decoded at its source; image null unbinds.
    void setTexture(std::shared_ptr<Image> image, std::uint64_t tex0, std::uint64_t clamp);

    static GaMeshRecorder &instance();

    // A GS register of a ga packet (alyssapan_drawga.h AlyssaPan_DrawGaRegister), and whether the draw
    // buffer is a display frame (AlyssaPan_DrawGaTarget): the meshes after them draw under them.
    void setRegister(int reg, std::uint64_t value);
    void setTarget(bool toDisplay);

    // The game's frame ends at gaWaitVSync's swap (alyssapan_drawga.h AlyssaPan_DrawGaSwap): what was
    // recorded since the last one becomes the last frame's. Answers true when there was anything (the
    // caller draws lastFrame()); a frame with nothing drawn leaves the display as it is.
    bool swap();

    // An object in draw order. Drawn to a GS frame that is not a display frame (the shadow buffer, the
    // effect buffer's HQDOF samples ..), it is not kept: the host has no such frame. The record calls answer
    // true when they closed a frame by the field instead of a swap (closeWithoutSwap).
    bool record(const AlyssaPanGaObject &object);

    // A full-screen fill in draw order (alyssapan_drawga.h AlyssaPan_DrawGaFill).
    bool recordFill(std::uint32_t rgba, int alphaFix);

    // A frame-buffer capture or copy in draw order (alyssapan_drawga.h).
    bool recordCopy(const Mesh &copy);

    // The Z buffer cleared in draw order (alyssapan_drawga.h AlyssaPan_DrawGaClearDepth).
    bool recordDepthClear();

    const std::vector<Mesh> &lastFrame() const { return m_last; }

private:
    static void project(Mesh &mesh, const AlyssaPanGaObject &object);
    static void cull(Mesh &mesh);
    bool close(std::uint64_t field);
    // A loop that draws and never swaps (none is known): its frame closes on the field, kSwaplessFields
    // after the last close, so that it is shown and cannot grow without end.
    bool closeWithoutSwap();

    std::vector<Mesh> m_frame;
    std::vector<Mesh> m_last;
    std::uint64_t m_closeField = 0;
    GsState m_gs;
    bool m_loggedFirst = false;
    bool m_loggedTexture = false;
    bool m_loggedCopy = false;
    bool m_loggedSwapless = false;
    std::shared_ptr<Image> m_image;
    std::uint64_t m_tex0 = 0;
    std::uint64_t m_clamp = 0;
};

} // namespace alyssapan::rendering
