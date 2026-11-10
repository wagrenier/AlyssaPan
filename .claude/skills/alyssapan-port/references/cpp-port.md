# The C++ port layer

The user's rule, carried from YamiPan (2026-11-03) and restated for AlyssaPan (2026-11-09): **the Pan
layer is modern, object-oriented C++; the game layer stays authentic.** AlyssaPan *builds on YamiPan's
C++ adaptation*: `src/alyssapan/` is YamiPan's `src/yamipan/` as of 93c73f0, renamed, with FS2's own
modules left behind; and it **reuses every shim the other Pans have** (MikuPan, MioPan, ReiPan, RosePan),
rewritten as classes here when they come over ("To carry over" below). Read a sibling to learn what a
feature must do and why, then write it as a class here; never paste C-style host code in.

## Scope

| tree | what it is | rule |
|---|---|---|
| `src/alyssapan/` | the Pan layer | modern OO C++ throughout: everything below applies in full |
| `src/sdk/` | SCE's API on the host | SCE's names and structs stay, they are the game's API. `.cpp` files are thin: state lives in `alyssapan::` classes |
| `src/iop/` | IOP code that runs on the host's IOP (none yet: SNDDRV.IRX, if it is the game's own) | authentic; functions carry `IOP 0x<addr>`, never `CT3 0x` |
| `src/game/`, `src/ga/` | the game layer (C, as the symtab names it) | **stays authentic**: reconstruction.md, the ROM's shapes, the game's file names (`movie.c`). This file does not apply |

Where game code reaches the host it does so through a few C seams it includes
(`alyssapan/rendering/alyssapan_drawui.h`, `alyssapan/os/alyssapan_time.h`,
`alyssapan/alyssapan_memory.h`): thin `AlyssaPan_*` forwarders onto the classes (`src/alyssapan/seams.cpp`).
CT3's own draw modules add their seams there, C-callable, when their TUs are read.

## How the code is written (YamiPan's rules, unchanged)

- C++23 (g++ 16). `#pragma once`. No force-included prefix header: a file includes what it uses.
- Names: `PascalCase` types, `camelCase` functions, `m_camelCase` members, `kPascalCase` constants,
  `snake_case.h` files. Everything host-side is in `namespace alyssapan` (sub-namespaces by directory).
- Ownership is RAII: `std::unique_ptr`, value members, the `gpu::` handles. No raw `new` / `delete` /
  `malloc` in host code. A class that owns a resource deletes or defines its copy operations.
- State lives in objects, not file-scope globals. The one service locator is `Application::instance()`,
  for the edge where game code (which cannot be given references) reaches the host; host classes take
  their collaborators by reference in the constructor.
- `enum class` over `#define` / plain enums; `constexpr` over macros; `std::span`, `std::string_view`,
  `std::filesystem::path`, `std::optional`, `std::expected` over pointer + size pairs, `char *` and
  out-parameters.
- Threads and locks from the standard library, except `os::Thread` for named threads with a chosen stack
  size. **A wait with a deadline or a duration goes through `os::TimedWaiter`**, never `wait_until` /
  `wait_for` / `std::this_thread::sleep_for`: under MinGW those land on the 15.6 ms system tick.
- Fallible start-up answers `std::expected<T, std::string>`; run-time trouble logs and degrades.
- Pragmatism beats ceremony: no interface with one implementation, no getter for a plain settings struct,
  no abstraction a second user does not need. Inheritance where there is a real family (`ui::Panel`).
- Comments say why, in full sentences. Carry over the siblings' explanations of behaviour that bit them.
  A comment that names FS2 describes what YamiPan learnt; when CT3 confirms or changes the behaviour,
  rewrite it for CT3.

## The map (what came from YamiPan)

```
src/main.cpp                     the process entry: Application(args).run()
src/common/alyssapan_stub.h      ALYSSAPAN_STUB / ALYSSAPAN_BRIDGE (the tools' contract, CT3 0x...)
src/lib/unassigned_stubs.cpp     stubs with no TU yet; today the game's main (gameMain)
src/sdk/                         libgraph, libdma, libvifpk, libpad, libcdvd, libmc, libmpeg, sif, fileio,
                                 eekernel, and the IOP libraries (iop.cpp, iop_host.h, libsd.h, thbase.h ...)
src/alyssapan/
  core/    Application, Logger (log::), Paths, Config / IniFile, Window, EventQueue, crash report
  os/      Thread, TimedWaiter
  ee/      Kernel (one EE thread runs at a time, by priority), Thread / Semaphore, InterruptController
  iop/     Kernel (one big lock), SystemMemory (2 MB below 4 GB), RpcServers, Spu2 (+ Spu2Voices)
  audio/   AudioOutput (one SDL stream, s16 stereo 48 kHz)
  gpu/     handles, Device       RAII over SDL_GPU, shader loading
  rendering/
           DisplayGeometry       THE source of display size and aspect (port feature #1)
           Renderer              the render thread: frames in, pictures out (2D path)
           UiCanvas              the 2D draw module, GS register values in (alyssapan_drawui.h)
           TextureStore          TIM2 pictures decoded at their source to RGBA8, cached by content
           PresentPass           grade + HDR output; Screenshotter the smoke shots
  media/   MpegStream, MpegDecoder  libmpeg on the host: PSS demux, FFmpeg MPEG-2
  io/      InputSystem (keyboard + gamepad -> DualShock 2, bindings), HostFiles, Disc
  ui/      Overlay, Panel family (display, audio, controls, performance, game data), Theme (CT3's colours)
```

Threads: as YamiPan's (main thread pumps the window; the render thread owns the GPU, ImGui and present
and re-presents the last frame through loads; EE threads one at a time; an interrupt thread for vblank
and alarms, PAL / NTSC from `sceGsResetGraph`; IOP threads under the big lock). **Check the EE model against
CT3** when main's TU is read: YamiPan chose "one EE thread at a time" for FS2's cooperative slots.

## Left in YamiPan (FS2's own; take them when CT3's code asks for the same thing)

- `rendering/model_renderer.*`, `yamipan_drawmodel.h`, the `model*.hlsl` shaders: rena's 3D draw module
  (FS2's batches, VU1 light formula, skinning, spot, shadows). CT3's `ga*` library gets its own module,
  written from its VU1 code; borrow the GPU plumbing (depth buffer, cached geometry, instance buffers).
- `rendering/frame_blend.*` (smooth motion past the game's frame rate, the user's choice "A" in FS2): the
  idea is generic, its records are FS2's. Comes back with CT3's 3D module (Frame::time is kept for it).
- `rendering/film_overlay_pass.*`, `frame_palette_pass.*`, `frame_ripple_pass.*`, `frame_filter_pass.*`,
  `yamipan_drawshadow.h`: FS2's whole-frame effects. CT3's `ss*` effects (blur, DOF, haze, wave) will want
  passes of the same shape: a pass between render passes that reads the scene target.
- `ui/teleport_panel.*` (F2): FS2's stage / event jump. CT3's equivalent is a debug panel over its own
  scene table, once that is read.
- `sdk/mcerror.*` (an FS2 game function), `src/iop/rxsnddrv/` (FS2's sound driver).

## To carry over from the siblings (the user's rule: every shim; open-items.md "Next up" orders them)

Survey of 2026-11-09 (MikuPan `src/mikupan`, MioPan `zero_rom/zero2np/src/miopan`, ReiPan `src/reipan`,
RosePan `src/rosepan` and their `src/sdk`). Each becomes a class here; the path is where to read it.

Platform, game-independent (1-5 carried over 2026-11-09, host task `alyssapan/core/window.cpp`):
1. ~~`gamecontrollerdb.txt`~~ `io::InputSystem::loadControllerMappings` reads `resources/gamecontrollerdb.txt`
   (RosePan's copy of the community DB) before the first pad opens.
2. ~~Window and exe icon~~ `Window::setIcon` (`resources/alyssapan.png`, `SDL_LoadPNG`) and `extern/icon.rc`
   (`extern/alyssapan.ico`); both drawn by `tools/make_icon.py` (original art: a clock face in the theme's
   colours; no Pillow needed).
3. ~~Pause and mute in the background~~ `Application::updateBackgroundState` after each pump, from
   `Window::inBackground` (flags, not focus events): mute is `Spu2Voices::setOutputMuted` ->
   `AudioOutput::setMuted` (the stream's gain: the mixer runs on); pause is
   `ee::InterruptController::setHeld` (fields skipped, not owed: no catch-up burst; alarms and timers run
   on). `[window] pause_in_background` (off), `[audio] mute_in_background` (on).
4. ~~Overlay DPI scale~~ `ui::Overlay::updateUiScale` before each frame: the window's display scale over its
   pixel density, or `[ui] scale` (0 = follow; `ALYSSAPAN_UI_SCALE`); both styles rebuilt from defaults.
5. ~~Cursor auto-hide~~ `ui::Overlay::hideIdleCursor`: still for `[ui] cursor_hide_seconds` (2), or
   borderless with the bar down, through `ImGui::SetMouseCursor(None)`.
6. Exclusive fullscreen, position restore and centring, size presets: MikuPan
   `mikupan_renderer.cpp:1610-1735`, `ui/mikupan_ui_settings.cpp:80-175` (and save the position: new).
7. The HDR swapchain (scRGB / HDR10, display headroom): RosePan `rosepan_renderer.cpp`
   (`SetHdrMode`, `GetHdrDisplayInfo`); the settings and PresentPass exist, the swapchain is SDR.
8. MSAA and anisotropy with their maxima: same RosePan file (`SetMsaaSamples`, `SetAnisotropy`).
9. GPU driver choice and debug layer: MikuPan `mikupan_gpu.cpp:771`.
10. User-directory override for test runs: ReiPan `reipan/io/reipan_paths.cpp:340` (`REIPAN_USER_DIR`).
11. Log level and assert hook: RosePan `rosepan/io/rosepan_log.cpp:586`.
12. Profiler phases and stutter attribution: RosePan `rosepan_profiler.cpp:103`, `rosepan_ui.cpp:2327`.

PS2 platform, generic across games:
13. A real memory card (folders, the async `sceMcSync` handshake): RosePan `sdk/libmc.cpp`.
14. Reverb: RosePan `sdk/iop_reverb.cpp`. 15. Volume groups applied to the mix: RosePan `sdk/iop_voice.cpp`
    (`SetUserVolume`), `rosepan_config.cpp`. 16. Audio dump and `audiocheck.py`: RosePan.
17. Playing from a disc image (.iso): MioPan `miopan/io/miopan_iso9660.cpp`, MikuPan
    `io/mikupan_disc_image.cpp`; `io::Disc` already answers sector reads.
18. Save import / export (card images, PSU / MAX / XPS / CBS): MioPan `miopan/save/*.cpp`, `miopan_ui_saves.cpp`.
19. newlib's 31-bit `rand()`: RosePan `rosepan/os/rosepan_rand.cpp` (CT3 links newlib: likely needed).
20. Frame limiter / vblank-wait harness: RosePan `os/rosepan_pacing.cpp`, MikuPan `mikupan_framelimiter.cpp`.
21. SDK pieces CT3 imports that YamiPan never needed: **`libsdr` (sdremote, CT3 uses it)**, `libvu0`
    functions, `libipu`, the EE timers (`eeregs.cpp`): RosePan / MioPan `sdk/`.
22. Movies: direct picture path and host-read movie stream (RosePan `rosepan_video.cpp`,
    `rosepan_moviestream.cpp`); movie audio through FFmpeg (MikuPan `av/mikupan_audio_decoder.cpp`).

Player-facing:
24. RmlUi menus (options, pause, game files, language): MioPan `miopan_ui/miopan_rml*.cpp`, `cmake/rmlui.cmake`.
25. Translations: MikuPan `mikupan_i18n.cpp`. 26. Gyro, mouse-look, pressure buttons, menu mouse: MioPan
    `io/miopan_input.cpp`, `miopan_menu_mouse.cpp`. 27. HD fonts, text overrides: MioPan
    `miopan_font_textures.cpp`, MikuPan `mikupan_textoverride.cpp`. 28. User screenshots, CRT filter:
    MikuPan. 29. HUD anchors, world past 4:3: MioPan `BeginAnchor`, RosePan `WidenProjection`.

Build and release: 32. CI (MioPan `.github/workflows/Nightly.yml`, MikuPan `Release.yml`); 33.
`CMakePresets.json`; AlyssaPan's own `extern/` (it borrows MioPan's prebuilt FFmpeg and shadercross today).

Not carried, on purpose: emulated EE RAM and the GS memory models (the user's rule: no GS emulator, not
even for textures).

## What is kept from the siblings' contract

`alyssapan.ini` next to `memcard/` in `%APPDATA%/Mikompilation/AlyssaPan/`, hand-editable, defaults < file
< environment; the `ALYSSAPAN_*` environment hooks (tools README "Platform contract"); the game thread's
name `AlyssaPanGame`; `[stub]`, `[W]`, `[E]` lines on stdout; shots in `ALYSSAPAN_SHOT_DIR`.
