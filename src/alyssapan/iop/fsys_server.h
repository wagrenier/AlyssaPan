// The IOP's file server, FSYS_IOP.IRX, on the host. CT3's EE binds three services of it (game/IOPGetFileSize.c:
// 'SUN@' a communication test, 'SUNA' a file's size and first sector, 'SUNB' "set SDT") and calls only the
// second: FlProc asks it for a loose file -- one not in DATA\CT3PACK.DAT, the movies of DATA\MOVIE1..3 --
// and then reads the file's sectors itself with sceCdRead. The IOP module's own code is not in the ELF, so
// the server answers from io::Disc, which lays the disc's folder out in sectors: the reply is the ROM's
// 0x10-byte record (game/IOPGetFileSize.h, result[0] size, [1] first sector). Written 2026-11-10 as a host
// task (the user's queue toward the first picture).
#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace alyssapan::iop
{

class Kernel;

class FsysServer
{
public:
    // The services' numbers, as the EE binds them (FSYS_ID_*).
    static constexpr std::uint32_t kComTest = 0x53554e40;    // 'SUN@'
    static constexpr std::uint32_t kGetFileSize = 0x53554e41; // 'SUNA'
    static constexpr std::uint32_t kSetSdt = 0x53554e42;      // 'SUNB'

    // The module's file on the disc, as sceSifLoadModule names it ("\IOP\FSYS_IOP.IRX;1").
    static constexpr std::string_view kModuleFile = "fsys_iop.irx";

    // The name record the EE sends (FSYS_NAME_SIZE) and the reply it reads.
    static constexpr int kNameSize = 0x40;
    static constexpr int kReplyWords = 4;

    // What a name that is not on the disc answers: no size, no sector. The IOP module's own answer is
    // unread (open-items.md); FlProc stores both words as they come.
    static constexpr std::int32_t kNotFound = -1;

    static FsysServer &instance();

    // The module's start: registers the three services and serves them (sceSifRpcLoop's effect).
    static int start();

    // 'SUNA': `name` ("\DATA\MOVIE1\S09.MOV;1") -> {size, first sector, 0, 0}.
    const std::array<std::int32_t, kReplyWords> &fileSize(std::string_view name);

private:
    FsysServer() = default;

    void registerServices(Kernel &kernel);

    static void *serve(std::uint32_t command, void *data, int size);

    alignas(16) std::array<char, kNameSize> m_name{};             // where a call's data is copied
    alignas(16) std::array<std::int32_t, kReplyWords> m_reply{};  // the reply the EE copies 0x10 bytes of
    bool m_registered = false;
};

} // namespace alyssapan::iop
