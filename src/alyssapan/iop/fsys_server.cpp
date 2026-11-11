#include "alyssapan/iop/fsys_server.h"

#include <algorithm>
#include <cstring>

#include "alyssapan/core/logger.h"
#include "alyssapan/io/disc.h"
#include "alyssapan/iop/kernel.h"

namespace alyssapan::iop
{

FsysServer &FsysServer::instance()
{
    static FsysServer server;
    return server;
}

int FsysServer::start()
{
    instance().registerServices(Kernel::instance());
    return 0;
}

// The module served its three numbers from one queue; the server object stands for the queue.
void FsysServer::registerServices(Kernel &kernel)
{
    if (m_registered)
        return;
    m_registered = true;
    for (const std::uint32_t number : {kComTest, kGetFileSize, kSetSdt})
        kernel.rpc().add(number, &FsysServer::serve, m_name.data(), this);
    kernel.rpc().serve(this);
}

const std::array<std::int32_t, FsysServer::kReplyWords> &FsysServer::fileSize(std::string_view name)
{
    m_reply.fill(0);
    std::uint32_t lsn = 0;
    std::uint32_t size = 0;
    if (io::Disc::instance().find(name, lsn, size))
    {
        m_reply[0] = static_cast<std::int32_t>(size);
        m_reply[1] = static_cast<std::int32_t>(lsn);
        log::info("fsys: {} -> {} bytes at sector {}", name, size, lsn);
    }
    else
    {
        m_reply[0] = kNotFound;
        m_reply[1] = kNotFound;
        log::info("fsys: {} not on the disc", name);
    }
    return m_reply;
}

// RpcServers copied the call's data into m_name; the reply is read from m_reply. The EE calls 'SUNA' with
// fno == the service number (IOPGetFileSize); the other two services are bound but never called, and
// answer a zero record.
void *FsysServer::serve(std::uint32_t command, void *data, int size)
{
    FsysServer &server = instance();
    if (command != kGetFileSize || data == nullptr || size <= 0)
    {
        server.m_reply.fill(0);
        return server.m_reply.data();
    }
    const char *name = static_cast<const char *>(data);
    const std::size_t length = strnlen(name, static_cast<std::size_t>(std::min(size, kNameSize)));
    return const_cast<std::int32_t *>(server.fileSize(std::string_view(name, length)).data());
}

} // namespace alyssapan::iop
