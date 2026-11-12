#include "alyssapan/iop/snd_driver_standin.h"

#include "alyssapan/core/logger.h"
#include "alyssapan/iop/kernel.h"

namespace alyssapan::iop
{

SndDriverStandIn &SndDriverStandIn::instance()
{
    static SndDriverStandIn driver;
    return driver;
}

int SndDriverStandIn::start()
{
    SndDriverStandIn &driver = instance();
    if (driver.m_registered)
        return 0;
    driver.m_registered = true;
    Kernel &kernel = Kernel::instance();
    kernel.rpc().add(kSndRpcId, &SndDriverStandIn::serve, driver.m_buffer.data(), &driver);
    kernel.rpc().serve(&driver);
    log::info("iop: SNDDRV.IRX stand-in (silent: the stream handshake only)");
    return 0;
}

// RpcServers copied the call's record into m_buffer; the EE copies its reply size from m_reply (zeros).
void *SndDriverStandIn::serve(std::uint32_t command, void *, int)
{
    SndDriverStandIn &driver = instance();
    driver.command(command & 0xffff0000u);
    return driver.m_reply.data();
}

void SndDriverStandIn::command(std::uint32_t kind)
{
    switch (kind)
    {
    case kCmdAdsEnable:
        m_flags |= kSndReady;
        sendEe(kEeSetSndFlags, m_flags);
        break;
    case kCmdAdsOpen:
        m_flags |= kAdsOpened;
        sendEe(kEeSetSndFlags, m_flags);
        break;
    case kCmdAdsStart:
        sendEe(kEeSetAdsActive, 1);
        break;
    case kCmdAdsStop:
    case kCmdAdsDisable:
        sendEe(kEeSetAdsActive, 0);
        break;
    default:
        break;
    }
}

// The driver's call to the EE's server: a record whose word +8 carries the value (_CpDivideEECom reads arg[2]).
void SndDriverStandIn::sendEe(std::uint32_t command, std::uint32_t value)
{
    Kernel &kernel = Kernel::instance();
    if (!kernel.rpc().serving(kEeComRpcId))
        return;
    alignas(16) std::array<std::uint32_t, 8> record{};
    record[2] = value;
    kernel.rpc().call(kernel, kEeComRpcId, command, record.data(), static_cast<int>(sizeof(record)));
}

} // namespace alyssapan::iop
