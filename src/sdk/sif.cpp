// sdk/sif.cpp -- the EE's side of the IOP.
//
// An IOP runs (alyssapan/iop/, carried over from YamiPan where FS2's RXSNDDRV runs on it): loading a module
// whose IOP code is reconstructed under src/iop/ runs its start() on the IOP kernel, IOP memory is
// iop::SystemMemory (an IOP address is a host address below 4 GB), SIF DMA and sceSifGetOtherData copy,
// and calls to a registered RPC server go through its own dispatcher, on the caller's thread, before
// sceSifCallRpc returns (so its end function runs at once). The other modules only "load", and a call
// to a server nobody registered answers at once with an empty reply. CT3 loads SNDDRV.IRX (its own
// driver), SCE's MODHSYN / MODMIDI / MODMSIN and SDRDRV: none is reconstructed yet (architecture.md
// "The IOP side"). FSYS_IOP.IRX, the file server, is host code (alyssapan/iop/fsys_server.h).
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cctype>
#include <mutex>
#include <string_view>
#include <thread>

#include "sdremote.h"
#include "sif.h"
#include "sifcmd.h"
#include "sifdev.h"
#include "sifman.h"
#include "sifrpc.h"

#include <cstdint>
#include <cstring>

#include "alyssapan/core/logger.h"
#include "alyssapan/iop/fsys_server.h"
#include "alyssapan/iop/kernel.h"

namespace
{

// Whether `path` ('cdrom0:\\IOP\\SNDDRV.IRX;1') names the module file `file` (lower case).
bool namesModule(std::string_view path, std::string_view file)
{
    return std::search(path.begin(), path.end(), file.begin(), file.end(), [](char a, char b) {
               return std::tolower(static_cast<unsigned char>(a)) == b;
           }) != path.end();
}

// The IOP modules whose code runs here: a module's file and its start(). A module reconstructed under
// src/iop/ (YamiPan's src/iop/rxsnddrv/ is the precedent: IOP code on iop::Kernel, its RPC server
// registered through sceSifRegisterRpc, functions annotated `IOP 0x...`), or a host server standing in
// for one whose code is not in the ELF.
struct IopModule
{
    std::string_view file;
    int (*start)();                     // the module's start, as Kernel::runModuleStart takes it
};
constexpr std::array<IopModule, 1> kIopModules{{
    // FSYS_IOP.IRX, the file server: host code (alyssapan/iop/fsys_server.h), not a reconstruction.
    {alyssapan::iop::FsysServer::kModuleFile, &alyssapan::iop::FsysServer::start},
}};

} // namespace

void AlyssaPan_IopStartModule(const char *path)
{
    if (path == nullptr)
        return;
    for (const IopModule &module : kIopModules)
    {
        if (!namesModule(path, module.file))
            continue;
        static std::mutex lock;
        std::scoped_lock started(lock);
        alyssapan::log::info("iop: starting {}", module.file);
        alyssapan::iop::Kernel::instance().runModuleStart(module.start);
        return;
    }
}

void sceSifInitRpc(unsigned int) {}
void sceSifExitCmd() {}
void sceSifInitIopHeap() {}
void sceFsReset() {}

int sceSifRebootIop(const char *)
{
    return 1;
}

int sceSifSyncIop()
{
    return 1;
}

int sceSifLoadModule(const char *path, int, const char *)
{
    // The IOP ran a module's start() as it loaded it.
    AlyssaPan_IopStartModule(path);

    static std::atomic<int> s_nextModule{1};
    return s_nextModule.fetch_add(1);
}

int sceSifBindRpc(sceSifClientData *bd, unsigned int request, unsigned int)
{
    // Callers wait for serve to be set; nothing reads through it. The number picks the server a call
    // goes to.
    bd->command = request;
    bd->serve = reinterpret_cast<_sif_serve_data *>(bd);
    return 0;
}

int sceSifCheckStatRpc(sceSifRpcData *)
{
    return 0;
}

// The IOP answered `rsize` bytes from the buffer its handler returned, whatever their meaning; so does this.
// A call nothing serves answers zeros.
int sceSifCallRpc(sceSifClientData *bd, unsigned int fno, unsigned int, void *send, int ssize, void *receive,
                  int rsize, sceSifEndFunc func, void *para)
{
    const void *reply = nullptr;
    if (bd != nullptr && AlyssaPan_IopRpcIsReady(bd->command) != 0)
        reply = AlyssaPan_IopRpcDispatch(bd->command, fno, send, ssize);
    if (receive != nullptr && rsize > 0)
    {
        if (reply != nullptr)
            std::memcpy(receive, reply, static_cast<std::size_t>(rsize));
        else
            std::memset(receive, 0, static_cast<std::size_t>(rsize));
    }
    if (func != nullptr)
        reinterpret_cast<void (*)(void *)>(func)(para);
    return 0;
}

// Both sides call it: the EE reading IOP memory (rxSndIopRead), the driver reading EE memory (src is the
// pointer RxSndDrvEePointer widened). Either way both are host pointers.
int sceSifGetOtherData(sceSifReceiveData *, void *src, void *dest, int size, unsigned int)
{
    if (dest == nullptr || size <= 0)
        return 0;
    if (src != nullptr)
        std::memcpy(dest, src, static_cast<std::size_t>(size));
    else
        std::memset(dest, 0, static_cast<std::size_t>(size));
    return 0;
}

int sceSifDmaStat(u_int)
{
    return -1;
}

// Both sides call it: the EE writing IOP memory (rxSndIopWrite), the driver writing EE memory
// (RxSndDrvSendToEe). The transfer is done when this returns; sceSifDmaStat says so.
u_int sceSifSetDma(sceSifDmaData *sdd, int len)
{
    if (sdd != nullptr)
    {
        for (int i = 0; i < len; i++)
        {
            if (sdd[i].addr != nullptr && sdd[i].data != nullptr && sdd[i].size > 0)
                std::memcpy(sdd[i].addr, sdd[i].data, static_cast<std::size_t>(sdd[i].size));
        }
    }
    return 1;
}

// Mode 1 is sysmem's high placement (the sound library's "from the top"), 0 its low one; mode 2 ("at
// `addr`") no caller uses, so it is placed low.
void *sceSifAllocSysMemory(int mode, u_int size, void *)
{
    using Placement = alyssapan::iop::SystemMemory::Placement;
    return alyssapan::iop::Kernel::instance().memory().allocate(size, mode == 1 ? Placement::High : Placement::Low);
}

int sceSifFreeSysMemory(void *addr)
{
    return alyssapan::iop::Kernel::instance().memory().free(addr) ? 0 : -1;
}

// The IOP heap is sysmem's low placement (the heap server's AllocSysMemory). The answer is an IOP address,
// a host address below 4 GB, so the game keeps it in a u_int.
void *sceSifAllocIopHeap(unsigned int size)
{
    return alyssapan::iop::Kernel::instance().memory().allocate(size);
}

int sceSifFreeIopHeap(void *addr)
{
    return alyssapan::iop::Kernel::instance().memory().free(addr) ? 0 : -1;
}

// sdremote's bind (sdk/sdremote.h): no sdrdrv on the host, nothing to wait for.
int sceSdRemoteInit(void)
{
    return 0;
}

// sdremote's EE -> IOP copy (sdk/sdremote.h): the ROM's SIF DMA, done at once.
int sceSdTransToIOP(const void *ee, u_int iopAddr, u_int size, int mode)
{
    if (mode == 0)
        return -1;
    if (ee != nullptr && iopAddr != 0 && size != 0)
        std::memcpy(reinterpret_cast<void *>(static_cast<std::uintptr_t>(iopAddr)), ee, size);
    return 0;
}
