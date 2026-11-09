// The game's disc, made from the extracted folder (carried over from YamiPan). FS2's rx cdrom driver read an
// ISO 9660 file system from raw sectors and its archives were addressed by sector, so the host answers sector
// reads: a primary volume descriptor, a path table and a root directory built in memory, and the folder's
// files laid out behind them, two volumes for FS2's dual-layer DVD. CT3 is one layer, and what it opens sits
// in DATA\ and IOP\: the path table is still root-only (open-items.md "Next up", the Pan layer), and whether
// CT3 reads sectors at all (sceCdSearchFile + sceCdRead on CT3PACK.DAT) or only its FSYS_IOP file server is
// for its file TU to say.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <vector>

namespace alyssapan::io
{

class Disc
{
public:
    static constexpr std::size_t kSectorSize = 0x800;

    static Disc &instance();

    // Lays the folder out. Sector numbers stay valid for the process's life.
    void mount(const std::filesystem::path &folder);

    // Whole sectors from `lsn`, the disc's own numbering. Beyond the end reads as zeroes, as a pressed
    // disc's lead-out reads as nothing useful.
    bool read(std::uint32_t lsn, std::uint32_t sectors, std::byte *buffer);

    std::uint32_t layer1Start() const { return m_layer1Start; }

private:
    struct FileCloser
    {
        void operator()(std::FILE *file) const { std::fclose(file); }
    };

    struct Extent
    {
        std::uint32_t firstSector;
        std::uint32_t sectorCount;
        std::filesystem::path path;
        std::unique_ptr<std::FILE, FileCloser> file; // opened on first use
    };

    void buildVolume(std::uint32_t base, std::span<const std::filesystem::path> files, std::uint32_t &nextSector);
    void readExtent(Extent &extent, std::uint32_t sector, std::byte *buffer);

    std::mutex m_lock;
    std::vector<Extent> m_extents; // ordered by firstSector
    std::vector<std::byte> m_metadata;
    std::uint32_t m_metadataSectors = 0;
    std::vector<std::byte> m_layer1Metadata;
    std::uint32_t m_layer1Start = 0;
};

} // namespace alyssapan::io
