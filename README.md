# AlyssaPan

A function-by-function reconstruction of Clock Tower 3 (PS2, USA, `SLUS_206.33`), ported to PC -- the fifth
Pan port, after MioPan (Fatal Frame 2), ReiPan (Fatal Frame 3), RosePan (Rule of Rose) and YamiPan
(Forbidden Siren 2), with MikuPan (Fatal Frame 1) upstream of them all. No game data or executables are
distributed: dump them from your own disc.

Status: bootstrap. The PC layer is YamiPan's modern C++ one, carried over (window, render thread, 2D draw
module, TIM2 textures, present and grade, ImGui overlay, input, settings, EE and IOP kernels, SPU2 and
sound card, MPEG on FFmpeg); the build is green and a run opens the window and reports the game's `main`
as its first stub. Clock Tower 3's executable keeps its symbol table, so every function already has its
own name.

The workflow, the project's decisions and its accumulated knowledge live in the Claude Code skill
`.claude/skills/alyssapan-port/`; the tools in `tools/reverse/` (see its README).

## Build

Windows, MSYS2 ucrt64 (g++, cmake, ninja): `cd tools/reverse && python build.py` (configures on first use,
reusing a sibling checkout's fetched SDL3 / ImGui; FFmpeg and shadercross come prebuilt from
`../MioPan/extern` for now). `python build.py --run 15` starts the game for 15 seconds and saves screenshots
to `tools/reverse/work/shots/`.

The data folder is your disc's root: the one with `DATA\CT3PACK.DAT` and `IOP\` in it. AlyssaPan asks for it
on first launch, or reads `ALYSSAPAN_DATA_DIR`; settings are in `%APPDATA%\Mikompilation\AlyssaPan\alyssapan.ini`.
