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
#include "alyssapan/rendering/renderer.h"
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

// PORT: CT3's frame end is not read yet (no UiCanvas kick in the game's code): the first ga object after a
// vblank closes the previous frame, and its meshes go to the canvas as one layer, as the VU1 program would
// have sent them (window coordinates, RGBAQ). Squeezed about the centre at wide aspects: the projection
// widened, not stretched. Untextured, no depth test, no fog yet (open-items.md item 4).
void DrawGaFrame(const std::vector<alyssapan::rendering::GaMeshRecorder::Mesh> &meshes)
{
    using alyssapan::rendering::GaMeshRecorder;
    constexpr std::uint64_t kPrimTriangleGouraud = 3 | 1 << 3;
    constexpr float kWindowMax = 65535.0f;
    const void *list = &GaMeshRecorder::instance();
    auto &ui = canvas();

    ui.setSqueezed(list, true);
    ui.setPrimitive(list, kPrimTriangleGouraud, nullptr);
    ui.begin(list, ALYSSAPAN_UIV_RGBAQ);
    for (const GaMeshRecorder::Mesh &mesh : meshes) {
        for (std::size_t t = 0; t < mesh.drawn.size(); ++t) {
            if (!mesh.drawn[t])
                continue;
            for (int k = 0; k < 3; ++k) {
                const GaMeshRecorder::Vertex &v = mesh.vertices[mesh.triangles[t * 3 + k]];
                AlyssaPanUiVertex out{};
                std::memcpy(out.rgba, v.drawRgba.data(), sizeof(out.rgba));
                out.x = static_cast<std::uint16_t>(std::clamp(v.screen[0], 0.0f, kWindowMax));
                out.y = static_cast<std::uint16_t>(std::clamp(v.screen[1], 0.0f, kWindowMax));
                out.z = static_cast<std::uint32_t>(std::max(v.screen[2], 0.0f));
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

void AlyssaPan_DrawUiKick(const void *const *lists, int count)
{
    // The frame is kicked right after the field that ended its wait.
    canvas().kick(std::span(lists, static_cast<std::size_t>(count)),
                  alyssapan::ee::Kernel::instance().interrupts().lastFieldTime());
}
