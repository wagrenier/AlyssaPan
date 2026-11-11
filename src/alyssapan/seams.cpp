// The seams game code calls (cpp-port.md "Scope"): thin forwarders, so the game's side never changes.
// Carried over from YamiPan without FS2's 3D, shadow and whole-frame effect seams (cpp-port.md "Left in
// YamiPan"): CT3's own draw modules add theirs here when their TUs are read.
#include <chrono>
#include <memory>
#include <span>

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

void AlyssaPan_DrawGaObject(const AlyssaPanGaObject *object)
{
    alyssapan::rendering::GaMeshRecorder::instance().record(*object);
}

void AlyssaPan_DrawUiKick(const void *const *lists, int count)
{
    // The frame is kicked right after the field that ended its wait.
    canvas().kick(std::span(lists, static_cast<std::size_t>(count)),
                  alyssapan::ee::Kernel::instance().interrupts().lastFieldTime());
}
