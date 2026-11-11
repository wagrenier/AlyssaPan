// The seams game code calls (cpp-port.md "Scope"): thin forwarders, so the game's side never changes.
// Carried over from YamiPan without FS2's 3D, shadow and whole-frame effect seams (cpp-port.md "Left in
// YamiPan"): CT3's own draw modules add theirs here when their TUs are read.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
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
    renderer.canvas().bindImage(list, renderer.textures().imageOfTim2(tim2, picture));
}

void AlyssaPan_DrawUiSqueeze(const void *list, int on)
{
    canvas().setSqueezed(list, on != 0);
}

float AlyssaPan_DrawUiSqueezeFactor(void)
{
    return canvas().squeezeFactor();
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
                           int rows, int x, int y)
{
    canvas().drawMacroblocks(list, owner, picture, macroblocks, columns, rows, static_cast<float>(x),
                             static_cast<float>(y));
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
constexpr int kRegClamp1 = 0x08;
constexpr int kRegTex11 = 0x14;
constexpr int kRegAlpha1 = 0x42;
// PORT: CT3's ALPHA_1 / TEX1_1 for the ga objects are not read (the draw environment's TU): (Cs - Cd) x As +
// Cd, and bilinear magnification and minification, until they are.
constexpr std::uint64_t kGaAlphaBlend = 0x44;
constexpr std::uint64_t kGaTex1Linear = 1ull << 5 | 1ull << 6;

// PRIM: triangles, Gouraud, textured, blended.
constexpr std::uint64_t kPrimTriangle = 3;
constexpr std::uint64_t kPrimGouraud = 1 << 3;
constexpr std::uint64_t kPrimTextured = 1 << 4;
constexpr std::uint64_t kPrimBlended = 1 << 6;

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

std::uint16_t gaCanvasX(float windowX)
{
    const float x = (windowX / kGaWindowUnit - kGaCentreX + kCanvasCentre) * kGaWindowUnit;
    return static_cast<std::uint16_t>(std::clamp(x, 0.0f, kWindowMax));
}

std::uint16_t gaCanvasY(float windowY)
{
    const float y = ((windowY / kGaWindowUnit - kGaCentreY) * (kCanvasFrameHeight / kGaFrameHeight) + kCanvasCentre) *
                    kGaWindowUnit;
    return static_cast<std::uint16_t>(std::clamp(y, 0.0f, kWindowMax));
}

// PORT: CT3's frame end is not read yet (no UiCanvas kick in the game's code): the first ga object after a
// vblank closes the previous frame, and its meshes go to the canvas as one layer, as the VU1 program would
// have sent them (window coordinates, RGBAQ, ST), each with the texture bound when it came. Squeezed about
// the centre at wide aspects: the projection widened, not stretched. No depth test, no fog yet
// (open-items.md item 7).
void DrawGaFrame(const std::vector<alyssapan::rendering::GaMeshRecorder::Mesh> &meshes)
{
    using alyssapan::rendering::GaMeshRecorder;
    const void *list = &GaMeshRecorder::instance();
    auto &ui = canvas();

    ui.setSqueezed(list, true);
    ui.setRegister(list, kRegAlpha1, kGaAlphaBlend);
    ui.setRegister(list, kRegTex11, kGaTex1Linear);
    for (const GaMeshRecorder::Mesh &mesh : meshes) {
        if (mesh.fill) {
            // Over the whole window, not the GS's 512 x 448: the fill does not take the squeeze.
            ui.fillColor(list, mesh.fillRgba, mesh.fillFix);
            continue;
        }
        const bool textured = mesh.image != nullptr;
        const int tfx = static_cast<int>(mesh.tex0 >> kTex0TfxShift) & kTex0TfxMask;
        const bool texelAlpha = (mesh.tex0 >> kTex0TccShift & 1) != 0;
        if (textured) {
            ui.setRegister(list, kRegClamp1, mesh.clamp);
            ui.bindImage(list, mesh.image);
            ui.setPrimitive(list, kPrimTriangle | kPrimGouraud | kPrimTextured | kPrimBlended, nullptr);
            ui.begin(list, ALYSSAPAN_UIV_RGBAQ | ALYSSAPAN_UIV_UV);
        } else {
            ui.setPrimitive(list, kPrimTriangle | kPrimGouraud, nullptr);
            ui.begin(list, ALYSSAPAN_UIV_RGBAQ);
        }
        const float texelsU = textured ? static_cast<float>(mesh.image->width) * kGaWindowUnit : 0.0f;
        const float texelsV = textured ? static_cast<float>(mesh.image->height) * kGaWindowUnit : 0.0f;
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
                // PORT: HIGHLIGHT / HIGHLIGHT2 (Ct x Cf + Af) are drawn as MODULATE until a screen needs them.
                out.x = gaCanvasX(v.screen[0]);
                out.y = gaCanvasY(v.screen[1]);
                out.z = static_cast<std::uint32_t>(std::max(v.screen[2], 0.0f));
                out.u = static_cast<std::uint16_t>(std::clamp(v.drawSt[0] * texelsU, 0.0f, kTexelMax));
                out.v = static_cast<std::uint16_t>(std::clamp(v.drawSt[1] * texelsV, 0.0f, kTexelMax));
                ui.addVertex(list, out);
            }
        }
    }
    const void *lists[] = {list};
    ui.kick(std::span(lists, 1), alyssapan::ee::Kernel::instance().interrupts().lastFieldTime());
}

} // namespace

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

// TEX0's TW / TH / PSM, the frame the picture is read through.
void AlyssaPan_DrawGaTexture(const AlyssaPanGaTexture *texture)
{
    constexpr int kTwShift = 26, kThShift = 30, kPsmShift = 20, kTccShift = 34;
    constexpr std::uint64_t kLog2Mask = 0xf, kPsmMask = 0x3f;
    auto &recorder = alyssapan::rendering::GaMeshRecorder::instance();
    const std::uint64_t tex0 = texture->tex0;
    std::shared_ptr<alyssapan::rendering::Image> image =
        alyssapan::Application::instance().renderer().textures().imageOfGsTexture(
            static_cast<int>(tex0 >> kPsmShift & kPsmMask), texture->width, texture->height,
            static_cast<int>(tex0 >> kTwShift & kLog2Mask), static_cast<int>(tex0 >> kThShift & kLog2Mask),
            texture->pixels, texture->clut, (tex0 >> kTccShift & 1) != 0);
    recorder.setTexture(std::move(image), tex0, texture->clamp);
}

void AlyssaPan_DrawUiKick(const void *const *lists, int count)
{
    // The frame is kicked right after the field that ended its wait.
    canvas().kick(std::span(lists, static_cast<std::size_t>(count)),
                  alyssapan::ee::Kernel::instance().interrupts().lastFieldTime());
}
