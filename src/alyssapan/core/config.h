// The host's settings: window, picture, audio, controls. They deliberately live outside the game's save
// data, in alyssapan.ini next to memcard/ in the user directory, hand-editable. Precedence is defaults <
// file < environment, and a run with an environment override does not save, so a test run can never
// disturb what the player chose.
#pragma once

#include <filesystem>
#include <memory>
#include <string>

namespace alyssapan
{

class IniFile;

enum class WindowMode
{
    Windowed,
    Borderless, // desktop fullscreen: no video mode switch, so alt-tab is instant
    Exclusive,  // the display's own fullscreen at the desktop's mode: the driver owns the screen (MikuPan)
};

// A window position nobody has saved yet: the window is centred on its display instead. Far outside any
// real desktop, so it can share the position's ini keys (MikuPan kept separate state for this).
inline constexpr int kWindowUnplaced = -32768;

enum class RenderResolution
{
    MatchWindow, // the window's own pixels
    NativePs2,   // the PS2's picture height times the scale, whatever the window
    WindowScale, // the window times the scale
};

enum class UpscaleFilter
{
    Nearest,
    Linear,
};

// The shape of the picture. Auto is the window's: no bars, ever.
enum class AspectMode
{
    Auto,
    Ratio4x3,
    Ratio16x9,
    Ratio16x10,
    Ratio21x9,
    Custom,
};

enum class HdrMode
{
    Off,
    Auto,
    ScRgb,
    Hdr10,
};

enum class PresentMode
{
    VSync,
    Mailbox,
    Immediate,
};

struct DisplaySettings
{
    WindowMode windowMode = WindowMode::Windowed;
    int windowWidth = 1280;
    int windowHeight = 960;
    // The windowed window's top-left corner on the desktop, saved on quit; kWindowUnplaced centres it.
    int windowX = kWindowUnplaced;
    int windowY = kWindowUnplaced;
    AspectMode aspectMode = AspectMode::Auto;
    float aspectRatio = 16.0f / 9.0f;
    RenderResolution renderMode = RenderResolution::MatchWindow;
    float renderScale = 1.0f;
    UpscaleFilter upscaleFilter = UpscaleFilter::Linear;
    PresentMode presentMode = PresentMode::VSync;
    // SDL_GPU's backend by SDL's name ("direct3d12", "vulkan", "metal"); empty lets SDL choose. A driver
    // that cannot start falls back to SDL's choice. Both apply on the next start.
    std::string gpuDriver;
    bool gpuDebug = false; // the backend's validation layer: slow, for finding GPU misuse
    // Texture anisotropy on the linear samplers: 1 (off), 2, 4, 8 or 16 (rendering/sampler_table.h rounds
    // other values down to a power of two). Applies on the next frame.
    int anisotropy = 1;
    // Multisampling of the game's frame: 1 (off), 2, 4 or 8 (rendering/msaa_targets.h rounds other values down
    // to a power of two and to what the device grants). Applies on the next frame.
    int msaa = 1;
    // Between two game frames, the cameras, lights, models and bones blend from the older one's places to the newer
    // one's (rendering/frame_blend.h): the game still runs at 25 frames a second, the picture moves at the monitor's
    // rate.

    // The output transform: all at their defaults, the present pass is skipped.
    HdrMode hdrMode = HdrMode::Off;
    float hdrPaperWhite = 200.0f; // nits; 0 follows the OS's SDR white level
    float hdrPeak = 0.0f;         // nits; 0 follows the display
    float hdrExpansion = 0.35f;   // highlight lift, 0..1
    float hdrExpansionKnee = 0.75f;
    float gradeBrightness = 1.0f;
    float gradeContrast = 1.0f;
    float gradeGamma = 1.0f;
    float gradeSaturation = 1.0f;

    template <class Archive>
    void serialize(Archive &archive);
};

struct PathSettings
{
    std::string dataFolder;

    template <class Archive>
    void serialize(Archive &archive);
};

struct GameSettings
{
    bool skipIntro = false;

    template <class Archive>
    void serialize(Archive &archive);
};

// What the window does when nobody is looking at it (MioPan's [window] section).
struct WindowSettings
{
    // Holds the game's field clock while the window is minimised or unfocused: the game stops where it
    // is, as a paused console would. Off by default, as in MioPan.
    bool pauseInBackground = false;

    template <class Archive>
    void serialize(Archive &archive);
};

inline constexpr float kUiScaleMin = 0.5f;
inline constexpr float kUiScaleMax = 4.0f;

// The overlay (ImGui).
struct UiSettings
{
    // 0 follows the display's content scale, and a window dragged to another monitor follows it there;
    // anything else is the scale, fixed.
    float scale = 0.0f;
    // A cursor left still over the game hides after this long; 0 never hides it.
    float cursorHideSeconds = 2.0f;

    template <class Archive>
    void serialize(Archive &archive);
};

// Percent on top of the game's own mix: 100 is the level the game asks for.
struct AudioSettings
{
    int master = 100;
    int music = 100;
    int effects = 100;
    bool muteInBackground = true;

    template <class Archive>
    void serialize(Archive &archive);
};

class Config
{
public:
    DisplaySettings display;
    WindowSettings window;
    UiSettings ui;
    PathSettings paths;
    GameSettings game;
    AudioSettings audio;

    // Reads the file over the defaults, then the environment over that. A missing file is the first
    // run: the defaults stand and the file is written.
    void load(const std::filesystem::path &file);

    // False when the write failed. Does nothing, successfully, for a run with environment overrides.
    bool save() const;

    IniFile &ini() { return *m_ini; }

    Config();
    ~Config();

private:
    void applyEnvironment();

    std::filesystem::path m_file;
    std::unique_ptr<IniFile> m_ini;
    bool m_environmentOverride = false;
};

} // namespace alyssapan
