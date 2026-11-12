// sdk/libcdvd.cpp -- the drive and the clock. The disc is alyssapan::io::Disc, the data folder served as an image, so all the drive
// has to say is that a PS2 DVD is in it; the clock is the PC's.
#include <chrono>
#include <cstdint>
#include <cstdio>

#include "libcdvd.h"
#include "libscf.h"
#include "alyssapan/io/disc.h"

using alyssapan::io::Disc;

namespace
{

constexpr int kDriveReady = 0x06; // sceCdStatus: spinning, paused

u_char bcd(int value)
{
    return static_cast<u_char>((value / 10) << 4 | (value % 10));
}

} // namespace

int sceCdInit(int)
{
    return 1;
}

int sceCdMmode(int)
{
    return 1;
}

int sceCdGetDiskType()
{
    return SCECdPS2DVD;
}

int sceCdDiskReady(int)
{
    return SCECdComplete;
}

int sceCdStatus()
{
    return kDriveReady;
}

int sceCdGetError()
{
    return SCECdErNO;
}

// Reads finish inside the call: the file thread that calls it is the only one waiting.
int sceCdRead(u_int lsn, u_int sectors, void *buf, sceCdRMode *)
{
    return Disc::instance().read(lsn, sectors, static_cast<std::byte *>(buf)) ? 1 : 0;
}

int sceCdSync(int)
{
    return 0;
}

// Reads finish inside sceCdRead, so there is never one to break.
int sceCdBreak()
{
    return 1;
}

// The file's sector and size on the disc the folder is served as (Disc::find); 1 found, 0 not.
int sceCdSearchFile(sceCdlFILE *fp, const char *name)
{
    std::uint32_t lsn = 0;
    std::uint32_t size = 0;
    if (!Disc::instance().find(name, lsn, size))
        return 0;
    fp->lsn = lsn;
    fp->size = size;
    std::snprintf(fp->name, sizeof(fp->name), "%s", name);
    return 1;
}

// ---- streaming (sceCdSt*): the movie's stream file (CT3 game/AA_videoCallback.c) ----------------------
// The IOP's stream ring is not needed: sceCdStRead reads straight from the disc at the stream's cursor,
// inside the call. Every call succeeds (the ROM retries StStart / StStop until they answer non-zero).
namespace
{
std::uint32_t streamLsn = 0; // the next sector sceCdStRead hands out
} // namespace

int sceCdStInit(u_int, u_int, u_int)
{
    streamLsn = 0;
    return 1;
}

int sceCdStStart(u_int lbn, sceCdRMode *)
{
    streamLsn = lbn;
    return 1;
}

// The sectors read (`size` is in sectors); 0 past the disc's end.
int sceCdStRead(u_int size, u_int *buf, u_int, u_int *err)
{
    if (err != nullptr)
        *err = SCECdErNO;
    if (size == 0 || !Disc::instance().read(streamLsn, size, reinterpret_cast<std::byte *>(buf)))
        return 0;
    streamLsn += size;
    return static_cast<int>(size);
}

int sceCdStStop()
{
    return 1;
}

int sceCdTrayReq(int, u_int *traychk)
{
    if (traychk != nullptr)
        *traychk = 0;
    return 1;
}

int sceCdReadDvdDualInfo(int *on_dual, u_int *layer1_start)
{
    *on_dual = 1;
    *layer1_start = Disc::instance().layer1Start();
    return 1;
}

int sceCdReadClock(sceCdCLOCK *clock)
{
    const auto local = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
    const auto day = std::chrono::floor<std::chrono::days>(local);
    const std::chrono::year_month_day date{day};
    const std::chrono::hh_mm_ss time{std::chrono::floor<std::chrono::seconds>(local - day)};

    clock->stat = 0;
    clock->second = bcd(static_cast<int>(time.seconds().count()));
    clock->minute = bcd(static_cast<int>(time.minutes().count()));
    clock->hour = bcd(static_cast<int>(time.hours().count()));
    clock->pad = 0;
    clock->day = bcd(static_cast<int>(static_cast<unsigned>(date.day())));
    clock->month = bcd(static_cast<int>(static_cast<unsigned>(date.month())));
    clock->year = bcd(static_cast<int>(date.year()) % 100);
    return 1;
}

// The console's RTC keeps Japan time; sceCdReadClock above already answers local time.
void sceScfGetLocalTimefromRTC(sceCdCLOCK *) {}
