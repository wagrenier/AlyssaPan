// The game's 2D draw module. The game's 2D drawers keep every decision the ROM makes and hand them over as
// the GS register values they would have packed; this turns those values into host draws. No GS
// emulation: registers it has no use for are ignored, and nothing ever goes through GS memory.
//
// Each packet list the game draws into is a layer, recorded apart (a list can be reopened during a frame).
// At the frame's end the game's kick (FS2: Vif1PktManage::Kick) names the lists that hold draws, in chain
// order, and the layers are joined into one Frame for the render thread.
#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

#include "alyssapan/rendering/frame.h"

struct AlyssaPanUiVertex;

namespace alyssapan::rendering
{

class Renderer;

class UiCanvas
{
public:
    explicit UiCanvas(Renderer &renderer);

    void setRegister(const void *list, int reg, std::uint64_t value);
    void setPrimitive(const void *list, std::uint64_t prim, const std::uint8_t *sharedColor);
    void bindImage(const void *list, std::shared_ptr<Image> image);
    // The vertices that follow keep their shape at a display wider than 4:3: x is pulled towards the
    // frame's centre. For what is drawn centred; the factor comes from the display's PictureLayout.
    void setSqueezed(const void *list, bool squeezed);
    float squeezeFactor() const; // 1 at 4:3 and narrower, under 1 wider
    // The vertices and images that follow land in a 4:3 box, the largest that fits the display, centred: the
    // frame pulled towards its centre in x at a wider display (as setSqueezed), in y at a narrower one. For
    // what filled the TV's whole picture (a movie): its shape kept, bars around it, never stretched.
    void setFitted(const void *list, bool fitted);
    // The whole frame times factor, 0..4: what the GS does with Cd as its A and FIX as its C.
    void scaleFrame(const void *list, float factor);
    // The whole frame pushed away from a colour: (Cd - Cs) * As + Cd, as ALPHA_1 0x41 does.
    void contrastFrame(const void *list, std::uint32_t rgba);
    // The whole frame (the whole window, at any aspect) covered by a colour: (Cs - Cd) * FIX + Cd, FIX
    // 0x80 = the colour alone. rgba's r in the low byte, 0..255.
    void fillColor(const void *list, std::uint32_t rgba, std::uint8_t fixedAlpha);
    // The depth test and write of the draws that follow (the 3D path's TEST_1 / ZBUF_1; the 2D draws keep
    // DepthTest::Off). depthOnly: no colour, depth alone (AFAIL ZB_ONLY under ATST NEVER). scale turns a
    // vertex's z, the GS's Z, into 0..1: one over the Z buffer's largest value.
    void setDepth(const void *list, DepthTest test, bool write, bool depthOnly, float scale);
    // The draws that follow keep only the fragments TEST_1's alpha test fails: the second pass of an AFAIL
    // that still writes something for them (FB_ONLY: the colour, ZB_ONLY: the depth).
    void setAlphaFailPass(const void *list, bool failPass);
    // The whole frame's depth back to 0, the farthest, the colour untouched: the GS's depth-only clear sprite.
    void clearDepth(const void *list);
    void begin(const void *list, int format);
    void addVertex(const void *list, const AlyssaPanUiVertex &vertex);

    // A movie picture, or any image the game replaces each frame: its RGBA8 pixels stretched over the frame's
    // rect at (x, y), `width` x `height`, placed as the layer's vertices are (setSqueezed, setFitted).
    void drawImage(const void *list, std::shared_ptr<Image> image, float x, float y, float width, float height);

    // A movie's picture: `columns` x `rows` 16 x 16 RGBA32 macroblocks in column-major order, drawn as drawImage
    // does. `picture` counts the movie's decoded pictures, and the image is only rebuilt when it moves: a movie
    // runs at 25 pictures a second under a faster game loop, and each image is a GPU texture.
    void drawMacroblocks(const void *list, const void *owner, int picture, const void *macroblocks, int columns,
                         int rows, float x, float y, float width, float height);

    // Ends the frame: these layers, in this order, go to the renderer. `time`: when the field that released the
    // frame was due.
    void kick(std::span<const void *const> lists, std::chrono::steady_clock::time_point time);

private:
    enum class Primitive : std::uint8_t
    {
        Point,
        Line,
        LineStrip,
        Triangle,
        TriangleStrip,
        TriangleFan,
        Sprite,
    };

    struct Layer
    {
        // The GS context, as far as it decides anything here.
        std::uint64_t alpha = 0;
        std::uint64_t tex0 = 0; // TFX and TCC (the 3D path sets it; the 2D draws leave MODULATE)
        std::uint64_t clamp = 0;
        std::uint64_t tex1 = 0;
        std::uint64_t test = 0;
        std::uint64_t frame = 0;
        Scissor scissor;
        std::shared_ptr<Image> image;

        Primitive primitive = Primitive::Sprite;
        bool gouraud = false;
        bool textured = false;
        bool blended = false;
        bool hasSharedColor = false;
        std::uint32_t sharedColor = 0;
        int format = 0;
        std::vector<Vertex> run; // the vertices of the open primitive
        unsigned runCount = 0;
        float squeezeX = 1.0f; // of x about the frame's centre
        float squeezeY = 1.0f; // of y, the same
        std::optional<BlendMode> forcedBlend; // a whole-frame pass's, past what the GS context says
        DepthTest depthTest = DepthTest::Off;
        bool depthWrite = false;
        bool depthOnly = false;
        float depthScale = 0.0f;
        bool alphaFailPass = false;

        std::vector<Vertex> vertices;
        std::vector<DrawCall> draws;
    };

    Layer &layer(const void *list);
    static float placedX(const Layer &target, float x); // a frame point, squeezed as the layer says
    static float placedY(const Layer &target, float y);
    void emit(Layer &target, std::initializer_list<Vertex> triangleVertices);
    void assemble(Layer &target, const Vertex &vertex);
    void fillFrame(Layer &target, BlendMode blend, std::uint32_t color, std::uint8_t fixedAlpha);
    static DrawCall stateOf(const Layer &layer);

    Renderer &m_renderer;
    const void *m_movieOwner = nullptr;
    int m_moviePicture = -1;
    std::shared_ptr<Image> m_movieImage;
    std::mutex m_lock;
    std::unordered_map<const void *, Layer> m_layers;
    std::uint64_t m_frames = 0;
};

} // namespace alyssapan::rendering
