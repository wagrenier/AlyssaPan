// sdk/libdma.cpp -- the DMAC on the host: the channels exist and everything finishes at once. GS packets are
// not sent anywhere; the frame reaches the renderer through the draw modules. The scratchpad channels are
// real: toSPR / fromSPR copy between memory and the host scratchpad (sdk/eekernel.h), with SADR holding a
// host pointer into it (the ga library writes it by hand before each transfer, CT3 0x018a1598).
#include "libdma.h"

#include <array>
#include <cstring>

namespace
{

constexpr std::size_t kChannelCount = 10;
constexpr std::size_t kQword = 16;

std::array<sceDmaChan, kChannelCount> s_channels{};
sceDmaEnv s_env{};

bool isChannel(const sceDmaChan *d, int id)
{
    return d == &s_channels[static_cast<std::size_t>(id)];
}

u_char *scratchpadSide(sceDmaChan *d)
{
    return static_cast<u_char *>(const_cast<void *>(d->sadr));
}

// Interleave: tqwc qwords move, then sqwc are skipped on the memory side; the scratchpad side is packed.
void interleave(u_char *memory, u_char *spr, int qwc, bool toSpr)
{
    const int tqwc = s_env.tqwc != 0 ? s_env.tqwc : qwc;
    while (qwc > 0) {
        const int run = qwc < tqwc ? qwc : tqwc;
        const std::size_t bytes = static_cast<std::size_t>(run) * kQword;
        if (toSpr)
            std::memcpy(spr, memory, bytes);
        else
            std::memcpy(memory, spr, bytes);
        spr += bytes;
        memory += bytes + static_cast<std::size_t>(s_env.sqwc) * kQword;
        qwc -= run;
    }
}

} // namespace

int sceDmaReset(int)
{
    return 0;
}

sceDmaEnv *sceDmaGetEnv(sceDmaEnv *env)
{
    *env = s_env;
    return env;
}

int sceDmaPutEnv(sceDmaEnv *env)
{
    s_env = *env;
    return 0;
}

sceDmaChan *sceDmaGetChan(int id)
{
    return &s_channels.at(static_cast<std::size_t>(id));
}

// Chain mode: nothing on the host follows tags yet (no CT3 code seen so far chains into the scratchpad).
void sceDmaSend(sceDmaChan *, void *) {}

void sceDmaSendN(sceDmaChan *d, void *addr, int qwc)
{
    const std::size_t bytes = static_cast<std::size_t>(qwc) * kQword;
    if (isChannel(d, SCE_DMA_toSPR))
        std::memcpy(scratchpadSide(d), addr, bytes);
    else if (isChannel(d, SCE_DMA_fromSPR))
        std::memcpy(addr, scratchpadSide(d), bytes);
}

void sceDmaSendI(sceDmaChan *d, void *addr, int qwc)
{
    if (isChannel(d, SCE_DMA_toSPR))
        interleave(static_cast<u_char *>(addr), scratchpadSide(d), qwc, true);
}

void sceDmaRecvI(sceDmaChan *d, void *addr, int qwc)
{
    if (isChannel(d, SCE_DMA_fromSPR))
        interleave(static_cast<u_char *>(addr), scratchpadSide(d), qwc, false);
}

int sceDmaSync(sceDmaChan *, int, int)
{
    return 0;
}
