// The Pan side of textures: TIM2 pictures decoded at their source to RGBA8, ready for the render
// thread to turn into GPU textures (Renderer::uploadImages). Nothing passes through GS memory: no swizzle, no
// CLUT upload, no replay of the game's transfers.
#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <tuple>
#include <unordered_map>


namespace alyssapan::rendering
{

struct Image;

class TextureStore
{
public:
    TextureStore() = default;
    TextureStore(const TextureStore &) = delete;
    TextureStore &operator=(const TextureStore &) = delete;

    // Picture `picture` of the TIM2 file at `file`, decoded during the call: the game may free the file right
    // after the bind (FS2's UiDrawer::setImage did), so nothing here keeps a pointer into it. The answer shares
    // its pixels with every other frame that draws the same picture. Null when there is no picture to show.
    // CT3's texture format is not read yet: a non-TIM2 format gets its own entry beside this one, decoded at
    // its source the same way (SKILL.md "The user's rules").
    std::shared_ptr<Image> imageOfTim2(const void *file, int picture);

    // CT3's own textures (game/BRANCH_CLEAR.h SUN_TEX_ATTR, 2026-11-11): level 0 as raw GS pixels in `psm`'s
    // order, `width` x `height`, read through a (1 << tw) x (1 << th) frame; `clut` the CT32 CLUT TEX0.CBP
    // picks (CSM1 for 8-bit), null for a direct format. Decoded during the call, keyed on the content like a
    // TIM2 picture. Null for a format not read (ct3pack.py's decode_tex reads the same five).
    std::shared_ptr<Image> imageOfGsTexture(int psm, int width, int height, int tw, int th, const void *pixels,
                                            const std::uint32_t *clut, bool useAlpha);

    // The `width` x `height` cell of `source` at (x, y), as an image of its own: what CLAMP_1's REGION_REPEAT
    // reads (u = (u & MSK) | FIX, MSK + 1 a power of two), drawn with a plain repeat. Kept while its source
    // is; null when the cell is not inside the source.
    std::shared_ptr<Image> regionOf(const std::shared_ptr<Image> &source, int x, int y, int width, int height);

private:
    struct Region
    {
        std::weak_ptr<Image> source;
        std::shared_ptr<Image> image;
    };
    using RegionKey = std::tuple<const Image *, int, int, int, int>; // the source, x, y, width, height

    struct Entry
    {
        std::shared_ptr<Image> image;
        std::uint64_t lastUse = 0;
    };

    void evictOver(std::size_t byteBudget);

    std::mutex m_lock;
    std::unordered_map<std::uint64_t, Entry> m_entries; // by the picture's content, see imageOf
    std::map<RegionKey, Region> m_regions;
    std::size_t m_bytes = 0;
    std::uint64_t m_uses = 0;
    bool m_loggedUnreadable = false;
};

} // namespace alyssapan::rendering
