// sdk/libmc.cpp -- SCE's memory-card API in front of the host card (alyssapan/io/memory_card.h: folders
// <user dir>/memcard/mc<port>-<slot>/, RosePan's layout). CT3 links libmc's EE side at 0x01826d38..0x01828530
// (sceMcInit .. sceMcGetEntSpace); its game reaches eleven of these entry points from memcard code
// (McInit, mc_check*, mc_formatcheck*, mc_free_area, mc_load_new, mc_save_new, mc_save_normal,
// MemorycardUpdate). The rest are here for the siblings' callers and for completeness.
#include "libmc.h"

#include "alyssapan/io/memory_card.h"

using alyssapan::io::MemoryCards;

namespace
{

MemoryCards &cards()
{
    return MemoryCards::instance();
}

} // namespace

int sceMcInit()
{
    return cards().init();
}

int sceMcEnd()
{
    return cards().end();
}

int sceMcSync(int, int *cmd, int *result)
{
    return static_cast<int>(cards().sync(cmd, result));
}

int sceMcGetInfo(int port, int slot, int *type, int *free, int *format)
{
    MemoryCards::Info info;
    const int accepted = cards().getInfo(port, slot, &info);
    if (accepted == sceMcResSucceed)
    {
        if (type != nullptr)
            *type = info.type;
        if (free != nullptr)
            *free = info.free;
        if (format != nullptr)
            *format = info.format;
    }
    return accepted;
}

int sceMcGetDir(int port, int slot, const char *name, unsigned int, int maxent, sceMcTblGetDir *table)
{
    return cards().getDir(port, slot, name != nullptr ? name : "", table, maxent);
}

int sceMcOpen(int port, int slot, const char *name, int mode)
{
    return cards().open(port, slot, name != nullptr ? name : "", mode);
}

int sceMcClose(int fd)
{
    return cards().close(fd);
}

int sceMcRead(int fd, void *buff, int size)
{
    return cards().read(fd, buff, size);
}

int sceMcWrite(int fd, const void *buff, int size)
{
    return cards().write(fd, buff, size);
}

int sceMcMkdir(int port, int slot, const char *name)
{
    return cards().makeDirectory(port, slot, name != nullptr ? name : "");
}

int sceMcDelete(int port, int slot, const char *name)
{
    return cards().remove(port, slot, name != nullptr ? name : "");
}

int sceMcFormat(int port, int slot)
{
    return cards().format(port, slot);
}

// Synchronous, like the ROM's: one slot per port (no multitap).
int sceMcGetSlotMax(int port)
{
    if (port < 0 || port >= MemoryCards::kPorts)
        return sceMcErrPortSlot;
    return MemoryCards::kSlotsPerPort;
}

// Modes 0 / 1 / 2 are SEEK_SET / SEEK_CUR / SEEK_END.
int sceMcSeek(int fd, int offset, int mode)
{
    return cards().seek(fd, offset, static_cast<MemoryCards::SeekFrom>(mode == 1 || mode == 2 ? mode : 0));
}

int sceMcFlush(int fd)
{
    return cards().flush(fd);
}

int sceMcUnformat(int port, int slot)
{
    return cards().unformat(port, slot);
}

int sceMcChdir(int port, int slot, const char *newDir, char *curDir)
{
    return cards().changeDirectory(port, slot, newDir != nullptr ? newDir : "", curDir);
}

int sceMcRename(int port, int slot, const char *org, const char *newName)
{
    return cards().rename(port, slot, org != nullptr ? org : "", newName != nullptr ? newName : "");
}

int sceMcGetEntSpace(int port, int slot, const char *path)
{
    return cards().entrySpace(port, slot, path != nullptr ? path : "");
}
