// The Pan side of textures: TIM2 pictures decoded at their source to RGBA8, ready for the render
// thread to turn into GPU textures (Renderer::uploadImages). Nothing passes through GS memory: no swizzle, no
// CLUT upload, no replay of the game's transfers.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
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

private:
    struct Entry
    {
        std::shared_ptr<Image> image;
        std::uint64_t lastUse = 0;
    };

    void evictOver(std::size_t byteBudget);

    std::mutex m_lock;
    std::unordered_map<std::uint64_t, Entry> m_entries; // by the picture's content, see imageOf
    std::size_t m_bytes = 0;
    std::uint64_t m_uses = 0;
    bool m_loggedUnreadable = false;
};

} // namespace alyssapan::rendering
