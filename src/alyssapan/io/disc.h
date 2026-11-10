// The game's disc, made from the extracted folder (carried over from YamiPan). FS2's rx cdrom driver read an
// ISO 9660 file system from raw sectors and its archives were addressed by sector, so the host answers sector
// reads: a primary volume descriptor, a path table and a root directory built in memory, and the folder's
// files laid out behind them, two volumes for FS2's dual-layer DVD. CT3 is one layer and reads sectors:
// FlInit finds DATA\CT3PACK.DAT with sceCdSearchFile and FlProc reads the pack's files with sceCdRead
// (game/ArtoolDebugOpen.c, 2026-11-10). The subfolders' files (DATA\, IOP\) get sectors too and are found
// by name (find); the ISO records still list only the root (no CT3 code reads them).
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
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

    // A file's first sector and size by its disc path ("\DATA\CT3PACK.DAT;1": case, the leading '\'
    // and the version do not matter). sceCdSearchFile's answer.
    bool find(std::string_view discPath, std::uint32_t &lsn, std::uint32_t &size);

private:
    struct FileCloser
    {
        void operator()(std::FILE *file) const { std::fclose(file); }
    };

    struct Named
    {
        std::string name; // "DATA\CT3PACK.DAT"
        std::uint32_t lsn;
        std::uint32_t size;
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
    std::vector<Named> m_named;
    std::filesystem::path m_folder;
    std::vector<std::byte> m_metadata;
    std::uint32_t m_metadataSectors = 0;
    std::vector<std::byte> m_layer1Metadata;
    std::uint32_t m_layer1Start = 0;
};

} // namespace alyssapan::io
