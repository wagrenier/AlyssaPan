// A host stand-in for SNDDRV.IRX, Capcom's sound driver, until its IOP code is reconstructed under src/iop/.
// The EE talks to the driver through RPC service SND_RPC_ID (game/CpDivideEECom.c, CpCall_IOPSndDriver)
// and the driver answers through the EE's own server, EE_COM_RPC_ID (_CpDivideEECom), which writes the
// status words the game polls (gEEStat). With no server on SND_RPC_ID nothing ever answered, and the ADS
// stream (the title music, hhXcmd_PlayADS) waited in SD_ADS_WAIT_OPEN for EE_IOP_ADS_OPENED for good --
// holding gADSOpenSync at 1, so FlProc never read another sector: after NEW the scene loads (0A02, 0A01,
// E_00) never started and tnXcmd_LoadWait waited on E_00 with a black screen (open-items.md item 8).
//
// The stand-in plays nothing. It keeps the driver's handshake only: the status bits the EE waits for
// (EE_IOP_SND_READY after SND_CMD_ADS_ENABLE, EE_IOP_ADS_OPENED after SND_CMD_ADS_OPEN) and the stream's
// "playing" word (adsActive: 1 from SND_CMD_ADS_START until SND_CMD_ADS_STOP / _DISABLE), sent the way the
// driver sends them, through EE_COM_RPC_ID. Every reply is zeros, as sceSifCallRpc answered before. A
// silent stream that never ends is what a looping one looks like; a one-shot stream never reports its end
// (the driver's own code decides that). Written 2026-11-12 as a host task
// (alyssapan/core/newgame_trace.cpp); the real driver replaces it.
#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace alyssapan::iop
{

class Kernel;

class SndDriverStandIn
{
public:
    // The services, as game/CpDivideEECom.h numbers them.
    static constexpr std::uint32_t kSndRpcId = 0x77777777;   // SND_RPC_ID: CpCall_IOPSndDriver
    static constexpr std::uint32_t kEeComRpcId = 0x77777779; // EE_COM_RPC_ID: the driver -> EE commands

    // The commands the stand-in answers (the high half of the RPC's fno; CpDivideEECom.h SND_CMD_*).
    static constexpr std::uint32_t kCmdAdsEnable = 0x2a0000;
    static constexpr std::uint32_t kCmdAdsOpen = 0x2b0000;
    static constexpr std::uint32_t kCmdAdsStart = 0x2c0000;
    static constexpr std::uint32_t kCmdAdsStop = 0x2d0000;
    static constexpr std::uint32_t kCmdAdsDisable = 0x320000;

    // What it sends the EE (EE_COM_*: the record's word +8 is the value) and the status bits.
    static constexpr std::uint32_t kEeSetAdsActive = 0x30000;
    static constexpr std::uint32_t kEeSetSndFlags = 0x40000;
    static constexpr std::uint32_t kSndReady = 0x1;  // EE_IOP_SND_READY
    static constexpr std::uint32_t kAdsOpened = 0x2; // EE_IOP_ADS_OPENED

    // The module's file on the disc, as sceSifLoadModule names it ("\IOP\SNDDRV.IRX;1").
    static constexpr std::string_view kModuleFile = "snddrv.irx";

    // The largest record the EE sends (gEE_RetBuf / gEE_RpcArg are 0x1800 bytes).
    static constexpr int kBufferSize = 0x1800;

    static SndDriverStandIn &instance();

    // The module's start: registers SND_RPC_ID and serves it.
    static int start();

private:
    SndDriverStandIn() = default;

    static void *serve(std::uint32_t command, void *data, int size);

    void command(std::uint32_t kind);
    void sendEe(std::uint32_t command, std::uint32_t value);

    alignas(16) std::array<std::uint8_t, kBufferSize> m_buffer{}; // where a call's record is copied
    alignas(16) std::array<std::uint8_t, kBufferSize> m_reply{};  // zeros: no reply word is modelled
    std::uint32_t m_flags = 0;                                    // the driver's copy of sndFlags
    bool m_registered = false;
};

} // namespace alyssapan::iop
